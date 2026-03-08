#include <vector>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include "../backend/datatypes.hpp"

class StochasticSimulator {
    private:
        std::atomic<int> maxIterations{0};
        variation_dp records{};

        std::pair<double, double> predict_variation(MC& market_context);

        std::vector<std::vector<double>> stochastic_analysis(size_t SSN, Date max_date);
        std::vector<std::vector<double>> stochastic_analysis(size_t SSN, size_t ocupation, size_t location, Date max_date);

        std::vector<std::pair<double, double>> stochastic_analysis_properties(size_t SSN);
        std::vector<std::pair<double, double>> stochastic_analysis_properties(size_t SSN, size_t ocupation, size_t location);

    public:
        double get_loan_payment_odds_percent(double amount, double interest_rate, Date date);
        double get_estimated_loan_profit(double amount, double interest_rate, Date date);
        double get_best_interest_for_profit(double amount, Date date);
        double get_minimum_interest_for_profit(double amount, std::vector<double> interest_rates, Date date);
}