#pragma once

#include <vector>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include "datatypes.hpp"

#define N_SIMULATIONS 100000

// Replace records with a richer cache struct
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

LoanAnalysis analyze_loan(
    size_t SSN, double amount, std::vector<double> candidate_rates,
    Date start_date, Date end_date, int num_payments
);

class StochasticSimulator {
    private:
        std::pair<double, double> predict_variation(MC& market_context);
    public:
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
