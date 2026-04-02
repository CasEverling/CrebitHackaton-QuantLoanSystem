#include "crow.h"
#include "stocastic_simulation.hpp"
#include "database_manager.hpp"
#include "datatypes.hpp"
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>
#include <vector>
#include <string>
#include <stdexcept>

using json = nlohmann::json;

// ── Translation tables ────────────────────────────────────────────────
static const std::unordered_map<int, std::string> OCCUPATION_NAMES = {
    {1, "Freelancer"},   {2, "Uber Driver"},    {3, "Artist"},
    {4, "Entrepreneur"}, {5, "Delivery Rider"},  {6, "Tutor"},
    {7, "Consultant"},   {8, "Content Creator"}, {9, "Handyman"},
};
static const std::unordered_map<int, std::string> LOCATION_NAMES = {
    {1, "New York, NY"}, {2, "Los Angeles, CA"}, {3, "Chicago, IL"},
    {4, "Houston, TX"},  {5, "Miami, FL"},        {6, "Austin, TX"},
    {7, "Seattle, WA"},  {8, "Boston, MA"},       {9, "Denver, CO"},
};
static std::string occ_name(int id) {
    auto it = OCCUPATION_NAMES.find(id);
    return it != OCCUPATION_NAMES.end() ? it->second : "Other";
}
static std::string loc_name(int id) {
    auto it = LOCATION_NAMES.find(id);
    return it != LOCATION_NAMES.end() ? it->second : "Location " + std::to_string(id);
}

static const std::string DB_CONN =
    "dbname=loanDatabase user=postgres password=sua_nova_senha host=127.0.0.1 port=5432";

// ── date helpers ──────────────────────────────────────────────────────

static Date parse_date(const std::string& s) {
    // expects "YYYY-MM-DD"
    return Date{
        std::stoi(s.substr(8, 2)),
        std::stoi(s.substr(5, 2)),
        std::stoi(s.substr(0, 4))
    };
}

static std::string date_to_string(const Date& d) {
    char buf[11];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", d.year, d.month, d.day);
    return std::string(buf);
}

// ── path sampling ─────────────────────────────────────────────────────
// Converts raw daily-delta paths to cumulative balance paths,
// then samples at most MAX_PATHS for the frontend animation.

static constexpr int MAX_PATHS      = 200;
static constexpr int SWEEP_STEPS    = 12;

static json sample_cumulative_paths(const std::vector<std::vector<double>>& raw) {
    int n      = std::min((int)raw.size(), MAX_PATHS);
    json paths = json::array();

    for (int i = 0; i < n; i++) {
        json path  = json::array();
        double cum = 0.0;
        for (double delta : raw[i]) {
            cum += delta;
            path.push_back(std::round(cum * 100.0) / 100.0);
        }
        paths.push_back(path);
    }
    return paths;
}

// ── interest rate sweep ───────────────────────────────────────────────

static json build_sweep(
    StochasticSimulator& sim,
    size_t ssn,
    double amount,
    double max_rate,
    Date start_date,
    Date end_date,
    int num_payments
) {
    json sweep = json::array();

    for (int i = 0; i <= SWEEP_STEPS; i++) {
        double rate = (max_rate / SWEEP_STEPS) * i;

        double odds   = sim.get_loan_payment_odds_percent(ssn, amount, rate, start_date, end_date, num_payments);
        double profit = sim.get_estimated_loan_profit    (ssn, amount, rate, start_date, end_date, num_payments);

        sweep.push_back({
            {"interest_rate",           std::round(rate   * 10000.0) / 10000.0},
            {"repayment_probability",   std::round(odds   * 10.0)    / 1000.0 },  // % -> 0-1
            {"estimated_profit",        std::round(profit * 100.0)   / 100.0  }
        });
    }

    return sweep;
}

// ── milestone pass-rates ──────────────────────────────────────────────

static json build_milestones(
    StochasticSimulator& sim,
    size_t ssn,
    double amount,
    double rate,
    Date start_date,
    Date end_date,
    int num_payments
) {
    std::vector<double> odds = sim.get_payment_schedule_odds(
        ssn, amount, rate, start_date, end_date, num_payments
    );

    // Reconstruct the same schedule the engine used so we have dates
    double total_repayment = amount * (1.0 + rate);
    double per_payment     = total_repayment / num_payments;
    int    total_days      = end_date - start_date;
    int    interval        = total_days / num_payments;

    json milestones = json::array();
    for (int p = 0; p < num_payments; p++) {
        Date payment_date = start_date + Date{interval * (p + 1), 0, 0};
        if (p == num_payments - 1) payment_date = end_date;   // snap last to end_date

        double pass_rate = (p < (int)odds.size()) ? odds[p] / 100.0 : 0.0;

        milestones.push_back({
            {"date",           date_to_string(payment_date)},
            {"payment_amount", std::round(per_payment * 100.0) / 100.0},
            {"pass_rate",      std::round(pass_rate  * 1000.0) / 1000.0}
        });
    }
    return milestones;
}

// ── main ──────────────────────────────────────────────────────────────

int main() {
    DataBaseManager::createDatabase();

    crow::SimpleApp app;
    StochasticSimulator sim;

    // ── GET /health ───────────────────────────────────────────────────
    CROW_ROUTE(app, "/health")([]() {
        return crow::response(200, "{\"status\":\"ok\"}");
    });

    // ── POST /analyze ─────────────────────────────────────────────────
    // Body: LoanRequest JSON
    // Returns: LoanResponse JSON
    CROW_ROUTE(app, "/analyze").methods(crow::HTTPMethod::POST)(
    [&sim](const crow::request& req) {
        crow::response res;
        res.add_header("Content-Type",                "application/json");
        res.add_header("Access-Control-Allow-Origin", "*");

        try {
            auto body = json::parse(req.body);

            // ── parse request ─────────────────────────────────────────
            size_t ssn          = body.at("ssn").get<size_t>();
            double amount       = body.at("loan_amount").get<double>();
            double max_rate     = body.at("max_interest_rate").get<double>();
            Date   end_date     = parse_date(body.at("end_date").get<std::string>());
            int    num_payments = body.value("num_payments", 1);

            // Derive start_date from today
            auto now     = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            std::tm* loc = std::localtime(&t);
            Date start_date{loc->tm_mday, loc->tm_mon + 1, loc->tm_year + 1900};

            // ── candidate rates for minimum viable ────────────────────
            std::vector<double> candidate_rates;
            candidate_rates.reserve(SWEEP_STEPS + 1);
            for (int i = 0; i <= SWEEP_STEPS; i++)
                candidate_rates.push_back((max_rate / SWEEP_STEPS) * i);

            // ── engine calls ──────────────────────────────────────────
            double best_rate = sim.get_best_interest_for_profit(
                ssn, amount, start_date, end_date, num_payments
            );
            double min_rate = sim.get_minimum_interest_for_profit(
                ssn, amount, candidate_rates, start_date, end_date, num_payments
            );
            double recommended_rate = std::min(best_rate, max_rate);

            double odds   = sim.get_loan_payment_odds_percent(
                ssn, amount, recommended_rate, start_date, end_date, num_payments
            );
            double profit = sim.get_estimated_loan_profit(
                ssn, amount, recommended_rate, start_date, end_date, num_payments
            );

            // ── raw paths for animation ───────────────────────────────
            json paths = json::array();
            {
                auto raw = sim.stochastic_analysis(ssn, end_date);
                int n    = std::min((int)raw.size(), MAX_PATHS);
                for (int i = 0; i < n; i++) {
                    json path  = json::array();
                    double cum = 0.0;
                    for (double delta : raw[i]) {
                        cum += delta;
                        path.push_back(std::round(cum * 100.0) / 100.0);
                    }
                    paths.push_back(path);
                }
            }

            // ── assemble response ─────────────────────────────────────
            json response = {
                {"viable",                    profit > 0.0},
                {"recommended_interest_rate", std::round(recommended_rate * 10000.0) / 10000.0},
                {"minimum_viable_rate",       min_rate >= 0.0 ? min_rate : json(nullptr)},
                {"statistics", {
                    {"repayment_probability", std::round(odds * 10.0) / 1000.0},
                    {"estimated_profit",      std::round(profit * 100.0) / 100.0}
                }},
                {"milestones",         build_milestones(sim, ssn, amount, recommended_rate,
                                                        start_date, end_date, num_payments)},
                {"interest_rate_sweep", build_sweep(sim, ssn, amount, max_rate,
                                                    start_date, end_date, num_payments)},
                {"paths", paths}
            };

            res.code = 200;
            res.body = response.dump();

        } catch (const json::exception& e) {
            res.code = 400;
            res.body = json{{"error", std::string("Invalid JSON: ") + e.what()}}.dump();
        } catch (const std::exception& e) {
            res.code = 500;
            res.body = json{{"error", e.what()}}.dump();
        }

        return res;
    });

    // ── OPTIONS /analyze  (CORS preflight) ───────────────────────────
    CROW_ROUTE(app, "/analyze").methods(crow::HTTPMethod::OPTIONS)(
    [](const crow::request&) {
        crow::response res(204);
        res.add_header("Access-Control-Allow-Origin",  "*");
        res.add_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");
        return res;
    });

    // ── POST /client  (register client data from Plaid) ───────────────
    // Body: { ssn, fixed_income: [{pay_day, amount, start_date}], location_id, activity_type }
    CROW_ROUTE(app, "/client").methods(crow::HTTPMethod::POST)(
    [](const crow::request& req) {
        crow::response res;
        res.add_header("Content-Type",                "application/json");
        res.add_header("Access-Control-Allow-Origin", "*");

        try {
            auto body    = json::parse(req.body);
            size_t ssn   = body.at("ssn").get<size_t>();

            // Fixed income streams
            if (body.contains("fixed_income")) {
                for (auto& fi : body["fixed_income"]) {
                    int    pay_day    = fi.at("pay_day").get<int>();
                    double amount     = fi.at("amount").get<double>();
                    Date   start_date = {
                        std::stoi(fi.at("start_date").get<std::string>().substr(8,2)),
                        std::stoi(fi.at("start_date").get<std::string>().substr(5,2)),
                        std::stoi(fi.at("start_date").get<std::string>().substr(0,4))
                    };
                    DataBaseManager::addFixedIncome(ssn, pay_day, amount, start_date);
                }
            }

            res.code = 201;
            res.body = json{{"status", "created"}, {"ssn", ssn}}.dump();

        } catch (const std::exception& e) {
            res.code = 400;
            res.body = json{{"error", e.what()}}.dump();
        }

        return res;
    });

    // ── OPTIONS /client ───────────────────────────────────────────────
    CROW_ROUTE(app, "/client").methods(crow::HTTPMethod::OPTIONS)(
    [](const crow::request&) {
        crow::response res(204);
        res.add_header("Access-Control-Allow-Origin",  "*");
        res.add_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");
        return res;
    });


    // ── GET /clients ──────────────────────────────────────────────────
    // Returns all clients for the frontend banner list.
    CROW_ROUTE(app, "/clients").methods(crow::HTTPMethod::GET)(
    []() {
        crow::response res;
        res.add_header("Content-Type",                "application/json");
        res.add_header("Access-Control-Allow-Origin", "*");
        try {
            pqxx::connection conn(DB_CONN);
            pqxx::work txn(conn);

            auto rows = txn.exec("SELECT DISTINCT CLIENT_SSN FROM Clients ORDER BY CLIENT_SSN");
            json clients = json::array();

            for (const auto& row : rows) {
                long long ssn = row[0].as<long long>();

                int job_id = 1;
                auto jr = txn.exec("SELECT ACTIVITY_TYPE FROM FreelancingIncome WHERE CLIENT_SSN = "
                    + txn.quote(ssn) + " AND END_DATE IS NULL ORDER BY START_DATE DESC LIMIT 1");
                if (!jr.empty()) job_id = jr[0][0].as<int>();

                int loc_id = 1;
                auto lr = txn.exec("SELECT LOCATION_ID FROM Location WHERE CLIENT_SSN = "
                    + txn.quote(ssn) + " ORDER BY MOVE_IN_DATE DESC LIMIT 1");
                if (!lr.empty()) loc_id = lr[0][0].as<int>();

                double fixed_total = 0.0;
                auto fr = txn.exec("SELECT COALESCE(SUM(AMOUNT),0) FROM FixedIncome WHERE CLIENT_SSN = "
                    + txn.quote(ssn) + " AND END_DATE IS NULL");
                if (!fr.empty()) fixed_total = fr[0][0].as<double>();

                double var_avg = 0.0;
                auto vr = txn.exec("SELECT COALESCE(AVG(BALANCE),0) FROM AccountBalanceVariableVariation "
                    "WHERE CLIENT_SSN = " + txn.quote(ssn) +
                    " AND DATE >= CURRENT_DATE - INTERVAL '30 days'");
                if (!vr.empty()) var_avg = vr[0][0].as<double>();

                bool connected = false;
                auto br = txn.exec("SELECT 1 FROM AccountBalance WHERE CLIENT_SSN = "
                    + txn.quote(ssn) + " LIMIT 1");
                connected = !br.empty();

                clients.push_back({
                    {"id",             std::to_string(ssn)},
                    {"name",           "Client " + std::to_string(ssn)},
                    {"occupation",     occ_name(job_id)},
                    {"location",       loc_name(loc_id)},
                    {"incomeFixed",    std::round(fixed_total * 100.0) / 100.0},
                    {"incomeVariable", std::round(var_avg * 30.0 * 100.0) / 100.0},
                    {"bankConnected",  connected}
                });
            }
            txn.commit();
            res.code = 200;
            res.body = clients.dump();
        } catch (const std::exception& e) {
            res.code = 500;
            res.body = json{{"error", e.what()}}.dump();
        }
        return res;
    });

    // ── OPTIONS /clients ──────────────────────────────────────────────
    CROW_ROUTE(app, "/clients").methods(crow::HTTPMethod::OPTIONS)(
    [](const crow::request&) {
        crow::response res(204);
        res.add_header("Access-Control-Allow-Origin",  "*");
        res.add_header("Access-Control-Allow-Methods", "GET, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");
        return res;
    });

    // ── POST /seed ────────────────────────────────────────────────────
    // Called by the Python fake data generator.
    // {
    //   "manager_ssn": 111, "client_ssn": 999,
    //   "location_id": 3, "job_id": 2,
    //   "fixed_income": [{"pay_day":15,"amount":2500,"start_date":"2024-01-15"}],
    //   "balances": [{"date":"2024-01-01","balance":1200.0}, ...]
    // }
    CROW_ROUTE(app, "/seed").methods(crow::HTTPMethod::POST)(
    [](const crow::request& req) {
        crow::response res;
        res.add_header("Content-Type",                "application/json");
        res.add_header("Access-Control-Allow-Origin", "*");
        try {
            auto body         = json::parse(req.body);
            size_t mgr_ssn    = body.at("manager_ssn").get<size_t>();
            size_t client_ssn = body.at("client_ssn").get<size_t>();
            int location_id   = body.at("location_id").get<int>();
            int job_id        = body.at("job_id").get<int>();

            DataBaseManager::addClient(mgr_ssn, client_ssn);

            {
                pqxx::connection conn(DB_CONN);
                pqxx::work txn(conn);
                txn.exec("INSERT INTO Location (CLIENT_SSN, LOCATION_ID, MOVE_IN_DATE) VALUES ("
                    + txn.quote((long long)client_ssn) + ", "
                    + txn.quote(location_id) + ", '2020-01-01') ON CONFLICT DO NOTHING");
                txn.exec("INSERT INTO FreelancingIncome (CLIENT_SSN, ACTIVITY_TYPE, START_DATE) VALUES ("
                    + txn.quote((long long)client_ssn) + ", "
                    + txn.quote(job_id) + ", '2020-01-01') ON CONFLICT DO NOTHING");
                txn.commit();
            }

            if (body.contains("fixed_income")) {
                for (const auto& fi : body.at("fixed_income")) {
                    std::string sd = fi.at("start_date").get<std::string>();
                    Date start{std::stoi(sd.substr(8,2)), std::stoi(sd.substr(5,2)), std::stoi(sd.substr(0,4))};
                    DataBaseManager::addFixedIncome(client_ssn,
                        fi.at("pay_day").get<int>(),
                        fi.at("amount").get<double>(), start);
                }
            }

            int count = 0;
            for (const auto& snap : body.at("balances")) {
                std::string sd = snap.at("date").get<std::string>();
                Date d{std::stoi(sd.substr(8,2)), std::stoi(sd.substr(5,2)), std::stoi(sd.substr(0,4))};
                DataBaseManager::addCreditInformation(client_ssn, d, snap.at("balance").get<double>());
                count++;
            }

            res.code = 200;
            res.body = json{{"status","ok"},{"client_ssn",client_ssn},{"balances_ingested",count}}.dump();

        } catch (const json::exception& e) {
            res.code = 400;
            res.body = json{{"error", std::string("Invalid JSON: ") + e.what()}}.dump();
        } catch (const std::exception& e) {
            res.code = 500;
            res.body = json{{"error", e.what()}}.dump();
        }
        return res;
    });

    // ── OPTIONS /seed ─────────────────────────────────────────────────
    CROW_ROUTE(app, "/seed").methods(crow::HTTPMethod::OPTIONS)(
    [](const crow::request&) {
        crow::response res(204);
        res.add_header("Access-Control-Allow-Origin",  "*");
        res.add_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");
        return res;
    });

    app.port(8080).multithreaded().run();
}
