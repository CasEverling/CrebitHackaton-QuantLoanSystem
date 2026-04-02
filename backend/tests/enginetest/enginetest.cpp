#include <iostream>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <vector>
#include <string>
#include <limits>
#include <pqxx/pqxx>
#include "../../stocastic_simulation/stocastic_simulation.hpp"
#include "../../database_management/database_manager.hpp"

// ── ANSI colors ───────────────────────────────────────────────────────
#define GREEN  "\033[32m"
#define RED    "\033[31m"
#define YELLOW "\033[33m"
#define CYAN   "\033[36m"
#define RESET  "\033[0m"

static const std::string CONN_STRING =
    "dbname=loanDatabase user=postgres password=sua_nova_senha host=127.0.0.1 port=5432";

// ── Test helpers ──────────────────────────────────────────────────────
int passed = 0;
int failed = 0;

void pass(const std::string& name) {
    std::cout << GREEN << "[PASS] " << RESET << name << "\n";
    passed++;
}
void fail(const std::string& name, const std::string& reason) {
    std::cout << RED << "[FAIL] " << RESET << name << " — " << reason << "\n";
    failed++;
}
void section(const std::string& name) {
    std::cout << "\n" << YELLOW << "──── " << name << " ────" << RESET << "\n";
}

// ── Helper: get today ─────────────────────────────────────────────────
static Date get_today() {
    auto now       = std::chrono::system_clock::now();
    std::time_t t  = std::chrono::system_clock::to_time_t(now);
    std::tm* local = std::localtime(&t);
    return Date{ local->tm_mday, local->tm_mon + 1, local->tm_year + 1900 };
}

// ── SSN picker ────────────────────────────────────────────────────────
struct ClientInfo {
    size_t ssn;
    int    balance_rows;
    int    variation_rows;
    double latest_balance;
    bool   has_job;
    bool   has_location;
};

static size_t pick_ssn() {
    std::vector<ClientInfo> clients;

    try {
        pqxx::connection conn(CONN_STRING);
        pqxx::work txn(conn);

        auto rows = txn.exec(R"(
            SELECT
                c.CLIENT_SSN,
                (SELECT COUNT(*) FROM AccountBalance ab
                 WHERE ab.CLIENT_SSN = c.CLIENT_SSN)            AS balance_rows,
                (SELECT COUNT(*) FROM AccountBalanceVariableVariation abvv
                 WHERE abvv.CLIENT_SSN = c.CLIENT_SSN)          AS variation_rows,
                (SELECT BALANCE FROM AccountBalance ab2
                 WHERE ab2.CLIENT_SSN = c.CLIENT_SSN
                 ORDER BY DATE DESC LIMIT 1)                     AS latest_balance,
                (SELECT COUNT(*) FROM FreelancingIncome fi
                 WHERE fi.CLIENT_SSN = c.CLIENT_SSN
                   AND fi.END_DATE IS NULL)                      AS has_job,
                (SELECT COUNT(*) FROM Location loc
                 WHERE loc.CLIENT_SSN = c.CLIENT_SSN)            AS has_location
            FROM Clients c
            ORDER BY c.CLIENT_SSN ASC
        )");
        txn.commit();

        for (const auto& row : rows) {
            ClientInfo ci;
            ci.ssn            = row[0].as<size_t>();
            ci.balance_rows   = row[1].as<int>();
            ci.variation_rows = row[2].as<int>();
            ci.latest_balance = row[3].is_null() ? 0.0 : row[3].as<double>();
            ci.has_job        = row[4].as<int>() > 0;
            ci.has_location   = row[5].as<int>() > 0;
            clients.push_back(ci);
        }
    } catch (const std::exception& e) {
        std::cerr << RED << "DB error fetching clients: " << e.what() << RESET << "\n";
        std::exit(1);
    }

    if (clients.empty()) {
        std::cerr << RED << "No clients found in database.\n" << RESET;
        std::exit(1);
    }

    std::cout << "\n" << CYAN << "Available clients:\n" << RESET;
    std::cout << "  " << std::left
              << std::setw(4)  << "#"
              << std::setw(16) << "SSN"
              << std::setw(10) << "Bal.rows"
              << std::setw(10) << "Var.rows"
              << std::setw(16) << "Latest bal."
              << std::setw(8)  << "Job"
              << std::setw(8)  << "Loc"
              << "\n";
    std::cout << "  " << std::string(70, '-') << "\n";

    for (int i = 0; i < (int)clients.size(); i++) {
        const auto& ci = clients[i];
        std::cout << "  " << std::left
                  << std::setw(4)  << (std::to_string(i + 1) + ".")
                  << std::setw(16) << ci.ssn
                  << std::setw(10) << ci.balance_rows
                  << std::setw(10) << ci.variation_rows
                  << std::setw(16) << std::fixed << std::setprecision(2) << ci.latest_balance
                  << std::setw(8)  << (ci.has_job      ? GREEN "yes" RESET : RED "no" RESET)
                  << std::setw(8)  << (ci.has_location ? GREEN "yes" RESET : RED "no" RESET)
                  << "\n";
    }

    int choice = 0;
    while (choice < 1 || choice > (int)clients.size()) {
        std::cout << "\nSelect client [1-" << clients.size() << "]: ";
        std::cin >> choice;
        if (std::cin.fail() || choice < 1 || choice > (int)clients.size()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << RED << "Invalid selection.\n" << RESET;
            choice = 0;
        }
    }

    size_t selected = clients[choice - 1].ssn;
    std::cout << GREEN << "Selected SSN: " << selected << RESET << "\n";
    return selected;
}

// ── Loan amount picker ────────────────────────────────────────────────
static double pick_loan_amount() {
    double amount = 0.0;
    while (amount <= 0.0) {
        std::cout << "\nLoan amount (R$): ";
        std::cin >> amount;
        if (std::cin.fail() || amount <= 0.0) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << RED << "Please enter a positive amount.\n" << RESET;
            amount = 0.0;
        }
    }
    return amount;
}

// ── DB and Engine Logic ───────────────────────────────────────────────

void test_engine_90_days(size_t SSN, double amount) {
    section("Engine — 90 Day Variable Daily Interest Simulation");

    StochasticSimulator sim;
    Date today    = get_today();
    Date end_date = today + Date{90, 0, 0};

    // Testing with the lower bound of interest (0.1% daily)
    double daily_low = 0.001; 
    double odds_low = sim.get_loan_payment_odds_percent(SSN, amount, daily_low, end_date);
    
    // Testing with the upper bound of interest (1.0% daily)
    double daily_high = 0.01;
    double odds_high = sim.get_loan_payment_odds_percent(SSN, amount, daily_high, end_date);

    std::cout << "  Odds @ 0.1% daily: " << std::fixed << std::setprecision(2) << odds_low << "%\n";
    std::cout << "  Odds @ 1.0% daily: " << std::fixed << std::setprecision(2) << odds_high << "%\n";

    if (odds_low >= odds_high)
        pass("Logic Check: Higher interest leads to lower or equal odds of repayment");
    else
        fail("Logic Check", "Higher interest showed higher repayment odds—check simulator variance");
}

void demo_output_daily_variation(size_t SSN, double amount) {
    section("90-Day Loan Odds: Daily Interest Analysis (0.1% to 1%)");

    StochasticSimulator sim;
    Date today    = get_today();
    Date end_date = today + Date{90, 0, 0};

    // Define the range of daily interest rates as requested
    std::vector<double> daily_rates = {
        0.001, 0.002, 0.004, 0.006, 0.008, 0.010
    };

    std::cout << "  Client SSN:   " << SSN    << "\n";
    std::cout << "  Loan amount:  R$ " << std::fixed << std::setprecision(2) << amount << "\n";
    std::cout << "  Duration:     90 days (Single Payment)\n\n";

    std::cout << "  " << std::left
              << std::setw(15) << "Daily Rate"
              << std::setw(12) << "Odds"
              << std::setw(18) << "Total Repayment"
              << "\n";
    std::cout << "  " << std::string(45, '-') << "\n";

    for (double rate : daily_rates) {
        double odds = sim.get_loan_payment_odds_percent(SSN, amount, rate, end_date);
        
        // Simple compounding formula: A = P(1 + r)^t
        double total_due = amount * std::pow((1.0 + rate), 90);

        std::cout << "  " 
                  << std::left << std::setw(15) << (std::to_string(rate * 100).substr(0, 4) + "%")
                  << std::setw(12) << (std::to_string((int)odds) + "%")
                  << "R$ " << std::fixed << std::setprecision(2) << total_due
                  << "\n";
    }

    // Calculating best interest within this specific daily range
    double best_daily = sim.get_best_interest_for_profit(SSN, amount, end_date);
    std::cout << "\n  Best daily rate for expected profit: " 
              << std::fixed << std::setprecision(3) << best_daily * 100.0 << "%\n";
}

int main() {
    std::cout << "\n=== StochasticSimulator: 90-Day Loan Suite ===\n";

    size_t SSN    = pick_ssn();
    double amount = pick_loan_amount();

    test_engine_90_days(SSN, amount);
    demo_output_daily_variation(SSN, amount);

    std::cout << "\n=== Simulation Complete ===\n\n";

    return 0;
}
