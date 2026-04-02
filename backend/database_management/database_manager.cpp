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
// ─────────────────────────────────────────────

static Date parse_pg_date(const std::string& s) {
    return Date{
        std::stoi(s.substr(8, 2)),
        std::stoi(s.substr(5, 2)),
        std::stoi(s.substr(0, 4))
    };
}

static std::string date_to_pg(const Date& d) {
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << d.year  << '-'
        << std::setw(2) << std::setfill('0') << d.month << '-'
        << std::setw(2) << std::setfill('0') << d.day;
    return oss.str();
}

// Advance to the next occurrence of pay_day in the following month.
static Date next_payday(const Date& current, int pay_day) {
    int m = current.month + 1;
    int y = current.year;
    if (m > 12) { m = 1; ++y; }
    std::tm t{};
    t.tm_mday = pay_day;
    t.tm_mon  = m - 1;
    t.tm_year = y - 1900;
    std::mktime(&t);
    return Date{t.tm_mday, t.tm_mon + 1, t.tm_year + 1900};
}

// ─────────────────────────────────────────────
// Static member definitions
// ─────────────────────────────────────────────
std::unordered_map<DataRequest, std::weak_ptr<std::vector<double>>>
    DataBaseManager::variance_cache;

std::unordered_map<DataRequest, std::weak_ptr<std::unordered_map<Date, double>>>
    DataBaseManager::pay_day_cache;

static const std::string CONN_STRING =
    "dbname=loanDatabase user=postgres password=sua_nova_senha host=127.0.0.1 port=5432";

static const std::string ADMIN_CONN_STRING =
    "dbname=postgres user=postgres password=sua_nova_senha host=127.0.0.1 port=5432";

// ─────────────────────────────────────────────
// createDatabase
// ─────────────────────────────────────────────
void DataBaseManager::createDatabase() {
    // Step 1: create the database itself — must be outside a transaction
    {
        pqxx::connection admin_conn(ADMIN_CONN_STRING);
        pqxx::nontransaction ntxn(admin_conn);
        auto result = ntxn.exec(
            "SELECT 1 FROM pg_Database WHERE datname = 'loanDatabase'"
        );
        if (result.empty())
            ntxn.exec("CREATE DATABASE loanDatabase");
    }

    // Step 2: create all tables
    {
        pqxx::connection conn(CONN_STRING);
        pqxx::work txn(conn);

        // Managers — institutions or advisors that own client accounts
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS Managers (
                SSN         BIGINT PRIMARY KEY NOT NULL,
                USERNAME    TEXT NOT NULL UNIQUE,
                PASSWORD    TEXT NOT NULL
            )
        )");

        // Clients — join table: manager owns client
        // CLIENT_SSN is not a FK to any table — clients have no login
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS Clients (
                MANAGER_SSN BIGINT NOT NULL REFERENCES Managers(SSN),
                CLIENT_SSN  BIGINT NOT NULL,
                PRIMARY KEY (MANAGER_SSN, CLIENT_SSN)
            )
        )");

        txn.exec("CREATE INDEX IF NOT EXISTS idx_clients_client ON Clients(CLIENT_SSN)");

        // FixedIncome — recurring income streams
        // END_DATE NULL = currently active
        // Natural key: (CLIENT_SSN, PAY_DAY, AMOUNT, START_DATE)
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS FixedIncome (
                ID          SERIAL PRIMARY KEY,
                CLIENT_SSN  BIGINT NOT NULL,
                PAY_DAY     INT NOT NULL CHECK (PAY_DAY BETWEEN 1 AND 31),
                AMOUNT      DECIMAL(15,2) NOT NULL,
                START_DATE  DATE NOT NULL,
                END_DATE    DATE,
                UNIQUE (CLIENT_SSN, PAY_DAY, AMOUNT, START_DATE)
            )
        )");

        // FreelancingIncome — job/activity history for class-level queries
        // END_DATE NULL = currently active
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS FreelancingIncome (
                ID              SERIAL PRIMARY KEY,
                CLIENT_SSN      BIGINT NOT NULL,
                ACTIVITY_TYPE   INT NOT NULL,
                START_DATE      DATE NOT NULL,
                END_DATE        DATE
            )
        )");

        // Location — residential history
        // MOVE_IN_DATE ensures prior-city data doesn't contaminate current parameters
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS Location (
                ID           SERIAL PRIMARY KEY,
                CLIENT_SSN   BIGINT NOT NULL,
                LOCATION_ID  INT NOT NULL,
                MOVE_IN_DATE DATE NOT NULL
            )
        )");

        // AccountBalance — absolute daily balance snapshot including fixed income
        // WEEK and DAY_OF_WEEK are generated columns — always in sync with DATE
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS AccountBalance (
                ID          SERIAL PRIMARY KEY,
                CLIENT_SSN  BIGINT NOT NULL,
                BALANCE     DECIMAL(15,2) NOT NULL,
                DATE        DATE NOT NULL,
                WEEK        INT NOT NULL GENERATED ALWAYS AS
                                (EXTRACT(WEEK FROM DATE)::INT) STORED,
                DAY_OF_WEEK INT NOT NULL GENERATED ALWAYS AS
                                (EXTRACT(DOW FROM DATE)::INT) STORED,
                UNIQUE (CLIENT_SSN, DATE)
            )
        )");

        // AccountBalanceVariableVariation — signed daily delta, fixed income stripped
        // BALANCE is a delta, not an absolute amount
        // This is the primary table for all 7 temporal component queries
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS AccountBalanceVariableVariation (
                ID          SERIAL PRIMARY KEY,
                CLIENT_SSN  BIGINT NOT NULL,
                BALANCE     DECIMAL(15,2) NOT NULL,
                DATE        DATE NOT NULL,
                WEEK        INT NOT NULL GENERATED ALWAYS AS
                                (EXTRACT(WEEK FROM DATE)::INT) STORED,
                DAY_OF_WEEK INT NOT NULL GENERATED ALWAYS AS
                                (EXTRACT(DOW FROM DATE)::INT) STORED,
                UNIQUE (CLIENT_SSN, DATE)
            )
        )");

        txn.exec("CREATE INDEX IF NOT EXISTS idx_abvv_ssn_date ON AccountBalanceVariableVariation(CLIENT_SSN, DATE)");
        txn.exec("CREATE INDEX IF NOT EXISTS idx_abvv_dow      ON AccountBalanceVariableVariation(DAY_OF_WEEK)");
        txn.exec("CREATE INDEX IF NOT EXISTS idx_ab_ssn_date   ON AccountBalance(CLIENT_SSN, DATE)");
        txn.exec("CREATE INDEX IF NOT EXISTS idx_fi_ssn        ON FixedIncome(CLIENT_SSN)");

        txn.commit();
    }
}

// ─────────────────────────────────────────────
// get_variance_history
// ─────────────────────────────────────────────
std::shared_ptr<std::vector<double>>
DataBaseManager::get_variance_history(const DataRequest& data) {
    auto it = variance_cache.find(data);
    if (it != variance_cache.end())
        if (auto cached = it->second.lock())
            return cached;

    pqxx::connection conn(CONN_STRING);
    pqxx::work txn(conn);

    std::string query;

    if (data.ssn != 0) {
        query = R"(
            SELECT BALANCE
            FROM AccountBalanceVariableVariation
            WHERE CLIENT_SSN = )" + txn.quote(static_cast<long long>(data.ssn)) + R"(
            ORDER BY DATE ASC
        )";
    } else {
        // Class-level: all clients matching this location + job
        // Respects MOVE_IN_DATE so prior-city data is excluded
        query = R"(
            SELECT abvv.BALANCE
            FROM AccountBalanceVariableVariation abvv
            JOIN Location loc
                ON abvv.CLIENT_SSN = loc.CLIENT_SSN
               AND abvv.DATE >= loc.MOVE_IN_DATE
            JOIN FreelancingIncome fi
                ON abvv.CLIENT_SSN = fi.CLIENT_SSN
               AND (fi.END_DATE IS NULL OR abvv.DATE <= fi.END_DATE)
               AND abvv.DATE >= fi.START_DATE
            WHERE loc.LOCATION_ID  = )" + txn.quote(static_cast<long long>(data.location)) + R"(
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

    variance_cache[data] = vec;
    return vec;
}

// ─────────────────────────────────────────────
// get_pay_days
// ─────────────────────────────────────────────
std::shared_ptr<std::unordered_map<Date, double>>
DataBaseManager::get_pay_days(const DataRequest& data) {
    auto it = pay_day_cache.find(data);
    if (it != pay_day_cache.end())
        if (auto cached = it->second.lock())
            return cached;

    pqxx::connection conn(CONN_STRING);
    pqxx::work txn(conn);

    std::string query = R"(
        SELECT START_DATE, END_DATE, PAY_DAY, AMOUNT
        FROM FixedIncome
        WHERE CLIENT_SSN = )" + txn.quote(static_cast<long long>(data.ssn)) + R"(
        ORDER BY START_DATE ASC
    )";

    auto result = txn.exec(query);
    txn.commit();

    auto map = std::make_shared<std::unordered_map<Date, double>>();

    for (const auto& row : result) {
        Date start    = parse_pg_date(row[0].as<std::string>());
        // END_DATE may be NULL for active streams — use a far-future sentinel
        Date end      = row[1].is_null()
                        ? Date{31, 12, 9999}
                        : parse_pg_date(row[1].as<std::string>());
        int  pay_day  = row[2].as<int>();
        double amount = row[3].as<double>();

        std::tm t{};
        t.tm_mday = pay_day;
        t.tm_mon  = start.month - 1;
        t.tm_year = start.year  - 1900;
        std::mktime(&t);
        Date d{t.tm_mday, t.tm_mon + 1, t.tm_year + 1900};

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
// ─────────────────────────────────────────────
size_t DataBaseManager::get_current_job(size_t clientSSN) {
    pqxx::connection conn(CONN_STRING);
    pqxx::work txn(conn);

    auto result = txn.exec(R"(
        SELECT ACTIVITY_TYPE
        FROM FreelancingIncome
        WHERE CLIENT_SSN = )" + txn.quote(static_cast<long long>(clientSSN)) + R"(
          AND END_DATE IS NULL
        ORDER BY START_DATE DESC
        LIMIT 1
    )");
    txn.commit();

    if (result.empty())
        throw std::runtime_error("No active job for CLIENT_SSN " + std::to_string(clientSSN));

    return result[0][0].as<size_t>();
}

// ─────────────────────────────────────────────
// get_current_location
// ─────────────────────────────────────────────
size_t DataBaseManager::get_current_location(size_t clientSSN) {
    pqxx::connection conn(CONN_STRING);
    pqxx::work txn(conn);

    auto result = txn.exec(R"(
        SELECT LOCATION_ID
        FROM Location
        WHERE CLIENT_SSN = )" + txn.quote(static_cast<long long>(clientSSN)) + R"(
        ORDER BY MOVE_IN_DATE DESC
        LIMIT 1
    )");
    txn.commit();

    if (result.empty())
        throw std::runtime_error("No location for CLIENT_SSN " + std::to_string(clientSSN));

    return result[0][0].as<size_t>();
}

// ─────────────────────────────────────────────
// addManager
// ─────────────────────────────────────────────
void DataBaseManager::addManager(size_t managerSSN,
                                  const std::string& username,
                                  const std::string& password) {
    pqxx::connection conn(CONN_STRING);
    pqxx::work txn(conn);

    txn.exec(
        "INSERT INTO Managers (SSN, USERNAME, PASSWORD) VALUES ("
        + txn.quote(static_cast<long long>(managerSSN)) + ", "
        + txn.quote(username) + ", "
        + txn.quote(password) + ")"
    );
    txn.commit();
}

// ─────────────────────────────────────────────
// addClient
// ─────────────────────────────────────────────
void DataBaseManager::addClient(size_t managerSSN, size_t clientSSN) {
    pqxx::connection conn(CONN_STRING);
    pqxx::work txn(conn);

    txn.exec(
        "INSERT INTO Clients (MANAGER_SSN, CLIENT_SSN) VALUES ("
        + txn.quote(static_cast<long long>(managerSSN)) + ", "
        + txn.quote(static_cast<long long>(clientSSN)) + ")"
    );
    txn.commit();
}

// ─────────────────────────────────────────────
// addFixedIncome
// ─────────────────────────────────────────────
void DataBaseManager::addFixedIncome(size_t clientSSN,
                                      int pay_day,
                                      double amount,
                                      const Date& start_date) {
    pqxx::connection conn(CONN_STRING);
    pqxx::work txn(conn);

    txn.exec(
        "INSERT INTO FixedIncome (CLIENT_SSN, PAY_DAY, AMOUNT, START_DATE) VALUES ("
        + txn.quote(static_cast<long long>(clientSSN)) + ", "
        + txn.quote(pay_day) + ", "
        + txn.quote(amount) + ", "
        + txn.quote(date_to_pg(start_date)) + ")"
    );
    txn.commit();
}

// ─────────────────────────────────────────────
// endFixedIncome
// Sets END_DATE = today on the matching active row.
// Matched on natural key: (CLIENT_SSN, PAY_DAY, AMOUNT, START_DATE)
// where END_DATE IS NULL.
// ─────────────────────────────────────────────
void DataBaseManager::endFixedIncome(size_t clientSSN,
                                      int pay_day,
                                      double amount,
                                      const Date& start_date) {
    pqxx::connection conn(CONN_STRING);
    pqxx::work txn(conn);

    // Get today's date from the DB to avoid clock skew between app and server
    auto today_result = txn.exec("SELECT CURRENT_DATE");
    std::string today = today_result[0][0].as<std::string>();

    auto result = txn.exec(
        "UPDATE FixedIncome SET END_DATE = " + txn.quote(today)
        + " WHERE CLIENT_SSN = " + txn.quote(static_cast<long long>(clientSSN))
        + "   AND PAY_DAY    = " + txn.quote(pay_day)
        + "   AND AMOUNT     = " + txn.quote(amount)
        + "   AND START_DATE = " + txn.quote(date_to_pg(start_date))
        + "   AND END_DATE IS NULL"
    );
    txn.commit();

    if (result.affected_rows() == 0)
        throw std::runtime_error(
            "No active FixedIncome found for CLIENT_SSN "
            + std::to_string(clientSSN)
            + " with pay_day=" + std::to_string(pay_day)
            + " amount=" + std::to_string(amount)
        );
}

// ─────────────────────────────────────────────
// addCreditInformation
//
// 1. Insert absolute balance into AccountBalance
// 2. Fetch the previous snapshot balance (most recent prior entry)
// 3. Compute delta = today - previous
// 4. Sum all active FixedIncome amounts where PAY_DAY == today.day
//    and subtract from delta (isolates variable income)
// 5. Insert adjusted delta into AccountBalanceVariableVariation
// ─────────────────────────────────────────────
void DataBaseManager::addCreditInformation(size_t clientSSN,
                                            const Date& today,
                                            double balance) {
    pqxx::connection conn(CONN_STRING);
    pqxx::work txn(conn);

    const std::string today_pg   = date_to_pg(today);
    const std::string client_q   = txn.quote(static_cast<long long>(clientSSN));

    // Step 1: insert today's absolute balance
    txn.exec(
        "INSERT INTO AccountBalance (CLIENT_SSN, BALANCE, DATE) VALUES ("
        + client_q + ", "
        + txn.quote(balance) + ", "
        + txn.quote(today_pg) + ")"
    );

    // Step 2: fetch the most recent previous balance and its date
    auto prev_result = txn.exec(R"(
        SELECT BALANCE, DATE FROM AccountBalance
        WHERE CLIENT_SSN = )" + client_q + R"(
          AND DATE < )" + txn.quote(today_pg) + R"(
        ORDER BY DATE DESC
        LIMIT 1
    )");

    // No previous snapshot — nothing to diff, skip variation insert
    if (prev_result.empty()) {
        txn.commit();
        return;
    }

    double prev_balance = prev_result[0][0].as<double>();
    Date   prev_date    = parse_pg_date(prev_result[0][1].as<std::string>());
    int    gap_days     = today - prev_date;   // operator- defined in date.hpp

    // If the gap is more than one day (e.g. weekend, missing Plaid sync),
    // spread the total delta evenly across the gap so each stored row
    // represents a single day's average variable movement.
    double delta = (balance - prev_balance) / (gap_days > 1 ? gap_days : 1);

    // Step 3: sum all active fixed income payments due today
    // A payment is due today if PAY_DAY == today.day and the stream is active
    // (START_DATE <= today and END_DATE IS NULL or END_DATE >= today)
    auto fi_result = txn.exec(R"(
        SELECT COALESCE(SUM(AMOUNT), 0)
        FROM FixedIncome
        WHERE CLIENT_SSN = )" + client_q + R"(
          AND PAY_DAY    = )" + txn.quote(today.day) + R"(
          AND START_DATE <= )" + txn.quote(today_pg) + R"(
          AND (END_DATE IS NULL OR END_DATE >= )" + txn.quote(today_pg) + R"()
    )");

    double fixed_today = fi_result[0][0].as<double>();
    double variable_delta = delta - fixed_today;

    // Step 4: insert the variable delta
    txn.exec(
        "INSERT INTO AccountBalanceVariableVariation (CLIENT_SSN, BALANCE, DATE) VALUES ("
        + client_q + ", "
        + txn.quote(variable_delta) + ", "
        + txn.quote(today_pg) + ")"
    );

    txn.commit();
}

double DataBaseManager::get_latest_balance(size_t SSN) {
    // This is a template implementation.
    // Replace with your specific DB driver calls (e.g., sqlite3_exec, pqxx, etc.)

    try {
        std::string query = "SELECT balance FROM account_history "
                            "WHERE ssn = " + std::to_string(SSN) + " "
                            "ORDER BY timestamp DESC LIMIT 1;";

        // Example pseudo-code for DB execution:
        // auto result = db.execute(query);
        // if (result.empty()) return 0.0;
        // return std::stod(result[0]["balance"]);

        // FOR DEMO/MOCK PURPOSES:
        return 1000.0;

    } catch (...) {
        // Fallback for simulation safety
        return 0.0;
    }
}
