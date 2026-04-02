#pragma once

#include "datatypes.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

// ─────────────────────────────────────────────
// DataBaseManager
//
// Schema overview:
//   Managers  — institutions / advisors that own client accounts
//   Clients   — join table linking managerSSN -> clientSSN
//   All financial tables reference clientSSN, never managerSSN
// ─────────────────────────────────────────────
class DataBaseManager {
private:
    static std::unordered_map<DataRequest, std::weak_ptr<std::vector<double>>>
        variance_cache;

    static std::unordered_map<DataRequest, std::weak_ptr<std::unordered_map<Date, double>>>
        pay_day_cache;

public:
    static void createDatabase();
    
    // ── Query ────────────────────────────────────────────────────────────
    static std::shared_ptr<std::vector<double>>
        get_variance_history(const DataRequest& data);

    static std::shared_ptr<std::unordered_map<Date, double>>
        get_pay_days(const DataRequest& data);

    static size_t get_current_job(size_t clientSSN);
    static size_t get_current_location(size_t clientSSN);
    static double get_latest_balance(size_t SSN);
    // ── Write ────────────────────────────────────────────────────────────

    // Register a new manager (bank, fintech, advisor)
    static void addManager(size_t managerSSN,
                           const std::string& username,
                           const std::string& password);

    // Associate a client SSN with a manager
    static void addClient(size_t managerSSN, size_t clientSSN);

    // Add a new active fixed income stream for a client.
    // start_date is part of the natural key — allows the same payday/amount
    // to exist across different employment periods.
    static void addFixedIncome(size_t clientSSN,
                               int pay_day,
                               double amount,
                               const Date& start_date);

    // Close an active fixed income stream by setting END_DATE = today.
    // Matched on (clientSSN, pay_day, amount, start_date) where END_DATE IS NULL.
    static void endFixedIncome(size_t clientSSN,
                               int pay_day,
                               double amount,
                               const Date& start_date);

    // Record today's absolute account balance for a client.
    //
    // Internally:
    //   1. Writes absolute balance to AccountBalance
    //   2. Fetches the previous snapshot balance
    //   3. Computes delta = today_balance - previous_balance
    //   4. Checks FixedIncome for any active row where pay_day == today.day
    //      and subtracts that fixed amount from the delta
    //   5. Writes adjusted delta to AccountBalanceVariableVariation
    //
    // This isolates variable income — fixed income is re-injected
    // deterministically during simulation.
    static void addCreditInformation(size_t clientSSN,
                                     const Date& today,
                                     double balance);
};
