#include "database_manager.hpp"
#include "./datatypes.hpp"
#include <pqxx/pqxx>
#include <memory>
#include <string>
#include <stdexcept>
#include <sstream>
#include <iomanip>

// ─────────────────────────────────────────────
// Date conversion helpers
// PostgreSQL DATE columns come back as "YYYY-MM-DD" strings.
// These two functions are the only place that translation happens.
// ─────────────────────────────────────────────

// "2024-03-15" → _date{15, 3, 2024}
static Date parse_pg_date(const std::string& s) {
    // s is guaranteed to be "YYYY-MM-DD" from PostgreSQL
    return Date{
        std::stoi(s.substr(8, 2)),   // day
        std::stoi(s.substr(5, 2)),   // month
        std::stoi(s.substr(0, 4))    // year
    };
}

// _date{15, 3, 2024} → "2024-03-15"
static std::string date_to_pg(const Date& d) {
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << d.year  << '-'
        << std::setw(2) << std::setfill('0') << d.month << '-'
        << std::setw(2) << std::setfill('0') << d.day;
    return oss.str();
}

// Advance to the next occurrence of pay_day within the following month.
// e.g. next_payday({1, 3, 2024}, 15) → {15, 4, 2024}
static Date next_payday(const Date& current, int pay_day) {
    int m = current.month + 1;
    int y = current.year;
    if (m > 12) { m = 1; ++y; }
    // mktime normalises out-of-range days (e.g. Feb 31 → Mar 3)
    std::tm t{};
    t.tm_mday = pay_day;
    t.tm_mon  = m - 1;
    t.tm_year = y - 1900;
    std::mktime(&t);
    return Date{t.tm_mday, t.tm_mon + 1, t.tm_year + 1900};
}

// Static member definitions
std::unordered_map<DataRequest, std::weak_ptr<std::vector<double>>>
    DataBaseManager::variance_cache;

std::unordered_map<DataRequest, std::weak_ptr<std::unordered_map<Date, double>>>
    DataBaseManager::pay_day_cache;

// ─────────────────────────────────────────────
// Connection helper — centralizes the connection string
// Change once here, affects everything
// ─────────────────────────────────────────────
static const std::string CONN_STRING =
    "dbname=loan_database user=postgres password=sua_nova_senha host=127.0.0.1 port=5432";

static const std::string ADMIN_CONN_STRING =
    "dbname=postgres user=postgres password=sua_nova_senha host=127.0.0.1 port=5432";

// ─────────────────────────────────────────────
// createDatabase
// NOTE: CREATE DATABASE cannot run inside a transaction in PostgreSQL.
// We use a separate nontransaction for that, then a normal transaction
// for all table creation.
// ─────────────────────────────────────────────
void DataBaseManager::createDatabase() {
    // Step 1: create the database itself (autocommit, no transaction)
    {
        pqxx::connection admin_conn(ADMIN_CONN_STRING);
        pqxx::nontransaction ntxn(admin_conn);

        // Only create if it doesn't exist — pg has no IF NOT EXISTS for CREATE DATABASE
        auto result = ntxn.exec(
            "SELECT 1 FROM pg_database WHERE datname = 'loan_database'"
        );
        if (result.empty()) {
            ntxn.exec("CREATE DATABASE loan_database");
        }
    }

    // Step 2: create all tables inside the new database
    {
        pqxx::connection conn(CONN_STRING);
        pqxx::work txn(conn);

        // Users
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS Users (
                SSN         BIGINT PRIMARY KEY NOT NULL,
                USERNAME    TEXT NOT NULL,
                PASSWORD    TEXT NOT NULL
            )
        )");

        // FixedIncome
        // Using DATE columns instead of separate DAY/MONTH/YEAR ints.
        // This makes all temporal queries dramatically simpler:
        //   EXTRACT(DOW FROM date), EXTRACT(MONTH FROM date), etc.
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS FixedIncome (
                ID          SERIAL PRIMARY KEY,
                SSN         BIGINT NOT NULL REFERENCES Users(SSN),
                PAY_DAY     INT NOT NULL,           -- day of month, 1–31
                AMOUNT      DECIMAL(15,2) NOT NULL,
                START_DATE  DATE NOT NULL,
                END_DATE    DATE NOT NULL
            )
        )");

        // FreelancingIncome
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS FreelancingIncome (
                ID              SERIAL PRIMARY KEY,
                SSN             BIGINT NOT NULL REFERENCES Users(SSN),
                ACTIVITY_TYPE   INT NOT NULL,
                START_DATE      DATE NOT NULL,
                END_DATE        DATE NOT NULL
            )
        )");

        // Location
        // MoveInDate as DATE ensures historical location data
        // doesn't contaminate current-location class parameters.
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS Location (
                ID          SERIAL PRIMARY KEY,
                SSN         BIGINT NOT NULL REFERENCES Users(SSN),
                LOCATION_ID INT NOT NULL,
                MOVE_IN_DATE DATE NOT NULL
            )
        )");

        // AccountBalance — full balance including fixed income
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS AccountBalance (
                ID              SERIAL PRIMARY KEY,
                SSN             BIGINT NOT NULL REFERENCES Users(SSN),
                BALANCE         DECIMAL(15,2) NOT NULL,
                DATE            DATE NOT NULL,
                WEEK            INT NOT NULL,
                DAY_OF_WEEK     INT NOT NULL CHECK (DAY_OF_WEEK BETWEEN 0 AND 6),
                BALANCE_REASON  TEXT NOT NULL CHECK (BALANCE_REASON IN ('fixed', 'volatile'))
            )
        )");

        // AccountBalanceVariableVariation
        // Fixed income excluded — this is the primary simulation input table.
        // All 7 temporal component queries run against this table.
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS AccountBalanceVariableVariation (
                ID              SERIAL PRIMARY KEY,
                SSN             BIGINT NOT NULL REFERENCES Users(SSN),
                BALANCE         DECIMAL(15,2) NOT NULL,
                DATE            DATE NOT NULL,
                WEEK            INT NOT NULL,
                DAY_OF_WEEK     INT NOT NULL CHECK (DAY_OF_WEEK BETWEEN 0 AND 6),
                BALANCE_REASON  TEXT NOT NULL
            )
        )");

        // Index on SSN + DATE for the temporal queries — these will be hot
        txn.exec("CREATE INDEX IF NOT EXISTS idx_abvv_ssn_date ON AccountBalanceVariableVariation(SSN, DATE)");
        txn.exec("CREATE INDEX IF NOT EXISTS idx_abvv_dow ON AccountBalanceVariableVariation(DAY_OF_WEEK)");

        txn.commit();
    }
}

// ─────────────────────────────────────────────
// get_variance_history
//
// Returns the variable balance variation history for a given DataRequest.
// If ssn == 0, returns class-level data (all users matching location + job).
// Results are weakly cached — simulation paths keep them alive, GC handles cleanup.
// ─────────────────────────────────────────────
std::shared_ptr<std::vector<double>>
DataBaseManager::get_variance_history(const DataRequest& data) {
    // Check cache first
    auto it = variance_cache.find(data);
    if (it != variance_cache.end()) {
        if (auto cached = it->second.lock())
            return cached;
    }

    pqxx::connection conn(CONN_STRING);
    pqxx::work txn(conn);

    std::string query;

    if (data.ssn != 0) {
        // Individual user history
        query = R"(
            SELECT BALANCE
            FROM AccountBalanceVariableVariation
            WHERE SSN = )" + txn.quote(static_cast<long long>(data.ssn)) + R"(
            ORDER BY DATE ASC
        )";
    } else {
        // Class-level history: all users matching this location + job
        // Joins through FreelancingIncome to filter by current job,
        // and through Location to filter by current location.
        // Only uses data from after the user's move-in date to avoid
        // contaminating current-location parameters with prior-city data.
        query = R"(
            SELECT abvv.BALANCE
            FROM AccountBalanceVariableVariation abvv
            JOIN Location loc
                ON abvv.SSN = loc.SSN
                AND abvv.DATE >= loc.MOVE_IN_DATE
            JOIN FreelancingIncome fi
                ON abvv.SSN = fi.SSN
            WHERE loc.LOCATION_ID = )" + txn.quote(static_cast<long long>(data.location)) + R"(
              AND fi.ACTIVITY_TYPE = )" + txn.quote(static_cast<long long>(data.job)) + R"(
            ORDER BY abvv.DATE ASC
        )";
    }

    auto result = txn.exec(query);
    txn.commit();

    auto vec = std::make_shared<std::vector<double>>();
    vec->reserve(result.size());
    for (const auto& row : result)
        vec->push_back(row[0].as<double>());

    variance_cache[data] = vec; // store as weak_ptr
    return vec;
}

// ─────────────────────────────────────────────
// get_pay_days
//
// Returns a map of {date -> fixed income amount} for all known pay days
// within the user's fixed income history.
// Used by the simulation to inject deterministic income at PayDay steps.
// ─────────────────────────────────────────────
std::shared_ptr<std::unordered_map<Date, double>>
DataBaseManager::get_pay_days(const DataRequest& data) {
    auto it = pay_day_cache.find(data);
    if (it != pay_day_cache.end()) {
        if (auto cached = it->second.lock())
            return cached;
    }

    pqxx::connection conn(CONN_STRING);
    pqxx::work txn(conn);

    // TODO: replace with your Date type's actual column mapping
    std::string query = R"(
        SELECT START_DATE, END_DATE, PAY_DAY, AMOUNT
        FROM FixedIncome
        WHERE SSN = )" + txn.quote(static_cast<long long>(data.ssn)) + R"(
        ORDER BY START_DATE ASC
    )";

    auto result = txn.exec(query);
    txn.commit();

    auto map = std::make_shared<std::unordered_map<Date, double>>();

    for (const auto& row : result) {
        Date start      = parse_pg_date(row[0].as<std::string>());
        Date end        = parse_pg_date(row[1].as<std::string>());
        int  pay_day    = row[2].as<int>();
        double amount   = row[3].as<double>();

        // First occurrence: clamp pay_day into the start month
        std::tm t{};
        t.tm_mday = pay_day;
        t.tm_mon  = start.month - 1;
        t.tm_year = start.year  - 1900;
        std::mktime(&t);
        Date d{t.tm_mday, t.tm_mon + 1, t.tm_year + 1900};

        // Walk month by month until past end date, accumulating amounts
        // (+=  handles overlapping FixedIncome rows for the same SSN)
        while (d <= end) {
            (*map)[d] += amount;
            d = next_payday(d, pay_day);
        }
    }

    pay_day_cache[data] = map;
    return map;
}

// ─────────────────────────────────────────────
// get_current_job
// Returns the most recent ACTIVITY_TYPE for the given SSN.
// ─────────────────────────────────────────────
size_t DataBaseManager::get_current_job(size_t SSN) {
    pqxx::connection conn(CONN_STRING);
    pqxx::work txn(conn);

    auto result = txn.exec(R"(
        SELECT ACTIVITY_TYPE
        FROM FreelancingIncome
        WHERE SSN = )" + txn.quote(static_cast<long long>(SSN)) + R"(
        ORDER BY START_DATE DESC
        LIMIT 1
    )");

    txn.commit();

    if (result.empty())
        throw std::runtime_error("No job found for SSN " + std::to_string(SSN));

    return result[0][0].as<size_t>();
}

// ─────────────────────────────────────────────
// get_current_location
// Returns the most recent LOCATION_ID for the given SSN.
// ─────────────────────────────────────────────
size_t DataBaseManager::get_current_location(size_t SSN) {
    pqxx::connection conn(CONN_STRING);
    pqxx::work txn(conn);

    auto result = txn.exec(R"(
        SELECT LOCATION_ID
        FROM Location
        WHERE SSN = )" + txn.quote(static_cast<long long>(SSN)) + R"(
        ORDER BY MOVE_IN_DATE DESC
        LIMIT 1
    )");

    txn.commit();

    if (result.empty())
        throw std::runtime_error("No location found for SSN " + std::to_string(SSN));

    return result[0][0].as<size_t>();
}