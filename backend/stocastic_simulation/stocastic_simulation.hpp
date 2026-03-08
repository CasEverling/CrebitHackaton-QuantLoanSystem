#pragma once

#include <vector>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <chrono>
#include <string>
#include "datatypes.hpp"

#define N_SIMULATIONS 100000

struct DayCache {
    double mean;
    double sd;
    double payday;
};

struct LoanAnalysis {
    double odds_percent;
    double estimated_profit;
    double best_rate;
    double minimum_viable_rate;
    std::vector<double> schedule_odds; // per milestone
};

// ── Simulation cache ──────────────────────────────────────────────────

struct SimulationResult {
    std::vector<std::vector<double>> paths;
    std::chrono::time_point<std::chrono::system_clock> created_at;
};

// ── Full API response struct (filled by analyze_loan) ─────────────────

struct MilestoneResult {
    std::string date;          // "YYYY-MM-DD"
    double payment_amount;
    double pass_rate;          // 0.0 – 1.0
};

struct InterestRateSweepEntry {
    double interest_rate;
    double repayment_probability;
    double estimated_profit;
};

struct LoanResponse {
    bool   viable;
    double recommended_interest_rate;
    double minimum_viable_rate;

    struct {
        double repayment_probability;
        double estimated_profit;
    } statistics;

    std::vector<MilestoneResult>        milestones;
    std::vector<InterestRateSweepEntry> interest_rate_sweep;
    std::vector<std::vector<double>>    paths;  // 200 sampled, cumulative balance
};

// ── Cache key ─────────────────────────────────────────────────────────

inline std::string make_cache_key(size_t ssn, const Date& end_date) {
    return std::to_string(ssn) + "_"
         + std::to_string(end_date.year)  + "_"
         + std::to_string(end_date.month) + "_"
         + std::to_string(end_date.day);
}

// ── Date formatting helper ────────────────────────────────────────────

inline std::string date_to_string(const Date& d) {
    char buf[11];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", d.year, d.month, d.day);
    return buf;
}

class StochasticSimulator {
private:
    // ── Simulation cache (SSN+date → paths) ──────────────────────────
    static std::unordered_map<std::string, SimulationResult> sim_cache;
    static std::shared_mutex cache_mutex;
    static constexpr int CACHE_TTL_SECONDS = 300; // 5 minutes

    // ── Core helpers ──────────────────────────────────────────────────
    std::pair<double, double> predict_variation(MC& market_context);

    // Returns cached paths or runs simulation and caches result
    const std::vector<std::vector<double>>& get_or_run_simulation(size_t SSN, Date end_date);

    std::vector<std::vector<double>> stochastic_analysis(size_t SSN, Date max_date);
    std::vector<std::vector<double>> stochastic_analysis(size_t SSN, size_t ocupation, size_t location, Date max_date);

    std::vector<std::pair<double, double>> stochastic_analysis_properties(size_t SSN, Date max_date);
    std::vector<std::pair<double, double>> stochastic_analysis_properties(size_t SSN, size_t ocupation, size_t location, Date max_date);

    std::vector<bool> evaluate_milestones(
        const std::vector<std::vector<double>>& paths,
        const std::vector<std::pair<Date, double>>& milestones,
        Date start_date,
        double interest_rate
    );

    std::vector<std::pair<Date, double>> build_installment_schedule(
        double total_repayment,
        Date start_date,
        Date end_date,
        int num_payments
    );

    std::vector<double> get_payment_schedule_odds(
        size_t SSN, double amount, double interest_rate,
        Date start_date, Date end_date, int num_payments
    );

    // ── Internal versions that accept pre-computed paths ──────────────
    double _odds_from_paths(const std::vector<std::vector<double>>& paths, double amount, double interest_rate, const std::vector<std::pair<Date, double>>& milestones, Date start_date);
    double _profit_from_paths(const std::vector<std::vector<double>>& paths, double amount, double interest_rate, const std::vector<std::pair<Date, double>>& milestones, Date start_date);
    double _best_rate_from_paths(const std::vector<std::vector<double>>& paths, double amount, const std::vector<std::pair<Date, double>>& milestones, Date start_date);
    double _min_rate_from_paths(const std::vector<std::vector<double>>& paths, double amount, std::vector<double> candidate_rates, const std::vector<std::pair<Date, double>>& milestones, Date start_date);
    std::vector<double> _schedule_odds_from_paths(const std::vector<std::vector<double>>& paths, const std::vector<std::pair<Date, double>>& milestones, Date start_date);

public:
    // ── Cache management ──────────────────────────────────────────────
    static void evict_expired_cache();
    static void clear_cache();

    // ── Primary API entry point ───────────────────────────────────────
    // Runs simulation once, computes all fields, returns full response.
    // This is what the Crow endpoint should call.
    LoanResponse analyze_loan(
        size_t SSN,
        double amount,
        double max_interest_rate,
        Date start_date,
        Date end_date,
        int num_payments
    );

    // ── Individual public methods (use cached paths internally) ───────
    double get_loan_payment_odds_percent(size_t SSN, double amount, double interest_rate, Date date);
    double get_estimated_loan_profit(size_t SSN, double amount, double interest_rate, Date date);
    double get_best_interest_for_profit(size_t SSN, double amount, Date date);
    double get_minimum_interest_for_profit(size_t SSN, double amount, std::vector<double> interest_rates, Date date);

    double get_loan_payment_odds_percent(size_t SSN, double amount, double interest_rate, const std::vector<std::pair<Date, double>>& milestones, Date start_date);
    double get_estimated_loan_profit(size_t SSN, double amount, double interest_rate, const std::vector<std::pair<Date, double>>& milestones, Date start_date);
    double get_best_interest_for_profit(size_t SSN, double amount, const std::vector<std::pair<Date, double>>& milestones, Date start_date);
    double get_minimum_interest_for_profit(size_t SSN, double amount, std::vector<double> interest_rates, const std::vector<std::pair<Date, double>>& milestones, Date start_date);

    double get_loan_payment_odds_percent(size_t SSN, double amount, double interest_rate, Date start_date, Date end_date, int num_payments);
    double get_estimated_loan_profit(size_t SSN, double amount, double interest_rate, Date start_date, Date end_date, int num_payments);
    double get_best_interest_for_profit(size_t SSN, double amount, Date start_date, Date end_date, int num_payments);
    double get_minimum_interest_for_profit(size_t SSN, double amount, std::vector<double> interest_rates, Date start_date, Date end_date, int num_payments);
};
