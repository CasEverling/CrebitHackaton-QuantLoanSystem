#include "stocastic_simulation.hpp"
#include "database_manager.hpp"
#include "datatypes.hpp"
#include <immintrin.h>

#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <cmath>
#include <functional>
#include <limits>
#include <vector>

#include <omp.h>

// ─────────────────────────────────────────────────────────────────────
// predict_variation
// ─────────────────────────────────────────────────────────────────────

std::pair<double, double> StochasticSimulator::predict_variation(
    MC& m,
    std::shared_ptr<std::vector<double>> user_information,
    std::shared_ptr<std::vector<double>> class_information)
{
    const std::vector<double>* primary = nullptr;
    if (user_information && user_information->size() >= 30)
        primary = user_information.get();
    else if (class_information && !class_information->empty())
        primary = class_information.get();

    if (!primary)
        return {0.0, 1.0};

    const auto& ch = *primary;
    const int   N  = static_cast<int>(ch.size());

    struct RowMeta { int dow; int week; int dom; int month; };
    std::vector<RowMeta> meta(N);
    {
        auto      now   = std::chrono::system_clock::now();
        std::time_t now_t = std::chrono::system_clock::to_time_t(now);

        for (int i = 0; i < N; i++) {
            std::time_t row_t  = now_t - (std::time_t)(N - 1 - i) * 86400;
            std::tm* row_tm = std::localtime(&row_t);
            meta[i] = {
                row_tm->tm_wday,
                static_cast<int>(row_tm->tm_yday / 7) + 1,
                row_tm->tm_mday,
                row_tm->tm_mon + 1
            };
        }
    }

    auto stats_of = [&](std::function<bool(int)> selector) -> std::pair<double, double> {
        double sum  = 0.0;
        double sum2 = 0.0;
        int    cnt  = 0;
        for (int i = 0; i < N; i++) {
            if (!selector(i)) continue;
            sum  += ch[i];
            sum2 += ch[i] * ch[i];
            cnt++;
        }
        if (cnt < 2) return {0.0, -1.0};
        double mean = sum / cnt;
        double var  = (sum2 / cnt) - (mean * mean);
        return { mean, std::sqrt(std::max(var, 0.0)) };
    };

    std::tm tmp_target{};
    tmp_target.tm_mday = m.day;
    tmp_target.tm_mon  = m.month - 1;
    tmp_target.tm_year = m.year  - 1900;
    std::mktime(&tmp_target);
    int target_week = static_cast<int>(tmp_target.tm_yday / 7) + 1;

    auto [dow_mean,   dow_sd]   = stats_of([&](int i){ return meta[i].dow   == m.day_of_week; });
    auto [week_mean,  week_sd]  = stats_of([&](int i){ return meta[i].week  == target_week;   });
    auto [dom_mean,   dom_sd]   = stats_of([&](int i){ return meta[i].dom   == m.day;         });
    auto [month_mean, month_sd] = stats_of([&](int i){ return meta[i].month == m.month;       });

    double total_mean = 0.0, total_sd = 1.0;
    {
        double sum = 0.0, sum2 = 0.0;
        for (int i = 0; i < N; i++) { sum += ch[i]; sum2 += ch[i] * ch[i]; }
        total_mean = sum / N;
        double var = (sum2 / N) - (total_mean * total_mean);
        total_sd   = std::sqrt(std::max(var, 0.0));
        if (total_sd < 1e-9) total_sd = 1.0;
    }

    struct Component { double mean; double sd; };
    std::vector<Component> components;
    components.reserve(5);

    auto add = [&](double mean, double sd) { if (sd > 0.0) components.push_back({mean, sd}); };
    add(dow_mean,   dow_sd);
    add(week_mean,  week_sd);
    add(dom_mean,   dom_sd);
    add(month_mean, month_sd);
    add(total_mean, total_sd);

    double weight_sum    = 0.0;
    double weighted_sum  = 0.0;
    for (const auto& c : components) {
        double w      = 1.0 / (c.sd * c.sd);
        weight_sum   += w;
        weighted_sum += w * c.mean;
    }

    double blended_mean = (weight_sum > 0.0) ? (weighted_sum / weight_sum) : total_mean;
    double blended_sd   = (weight_sum > 0.0) ? std::sqrt(1.0 / weight_sum) : total_sd;

    // Floor sd at 20% of total variance to ensure stochasticity
    blended_sd = std::max(blended_sd, total_sd * 0.20);

    return { blended_mean, blended_sd };
}

// ─────────────────────────────────────────────────────────────────────
// stochastic_analysis
// ─────────────────────────────────────────────────────────────────────

std::vector<std::vector<double>> StochasticSimulator::stochastic_analysis(
    size_t SSN, size_t ocupation, size_t location, Date pay_date)
{
    auto now      = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm* local = std::localtime(&t);

    Date curr_date{ local->tm_mday, local->tm_mon + 1, local->tm_year + 1900 };
    int m = pay_date - curr_date;

    if (m <= 0) return {};

    // NEW: Inject current balance so simulation starts from actual client status
    double starting_balance = 0.0;
    try {
        starting_balance = DataBaseManager::get_latest_balance(SSN);
    } catch (...) {
        starting_balance = 0.0;
    }

    auto return_value = std::vector<std::vector<double>>(
        N_SIMULATIONS, std::vector<double>(m, 0.0));

    DataRequest individual_req {0, SSN, location, ocupation};
    DataRequest class_req {0, 0, location, ocupation};

    auto individual_history = DataBaseManager::get_variance_history(individual_req);
    auto class_history      = DataBaseManager::get_variance_history(class_req);
    auto pay_days           = DataBaseManager::get_pay_days({ 1, SSN, location, ocupation });

    std::vector<DayCache> cache(m);
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < m; i++) {
        Date target = curr_date + Date{ i, 0, 0 };
        auto data = MC{ target.day, target.month, target.year, day_of_week(target), location, ocupation, SSN };
        auto [mean, sd] = predict_variation(data, individual_history, class_history);
        
        double payday_injection = 0.0;
        auto it = pay_days->find(target);
        if (it != pay_days->end()) payday_injection = it->second;

        cache[i] = { mean, sd, payday_injection };
    }

    #pragma omp parallel
    {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count()
                        ^ (std::hash<std::thread::id>{}(std::this_thread::get_id()));
        std::default_random_engine generator(seed);
        std::normal_distribution<double> normal(0.0, 1.0);

        #pragma omp for schedule(static)
        for (int i = 0; i < (int)N_SIMULATIONS; i++) {
            double current_bal = starting_balance;
            for (int j = 0; j < m; j++) {
                current_bal += (normal(generator) * cache[j].sd) + cache[j].mean + cache[j].payday;
                return_value[i][j] = current_bal;
            }
        }
    }

    return return_value;
}

// ─────────────────────────────────────────────────────────────────────
// Core Probability Functions (Daily Compounding)
// ─────────────────────────────────────────────────────────────────────

double StochasticSimulator::get_loan_payment_odds_percent(
    size_t SSN, double amount, double daily_interest_rate, Date date)
{
    auto paths = stochastic_analysis(SSN, 1, 1, date); // Simplified overload call
    if (paths.empty()) return 0.0;

    int days = (int)paths[0].size();
    double total_required = amount * std::pow(1.0 + daily_interest_rate, days);
    
    int successes = 0;
    #pragma omp parallel for reduction(+:successes) schedule(static)
    for (int i = 0; i < (int)paths.size(); i++) {
        if (paths[i].back() >= total_required) successes++;
    }

    return (100.0 * successes) / (double)paths.size();
}

double StochasticSimulator::get_estimated_loan_profit(
    size_t SSN, double amount, double daily_interest_rate, Date date)
{
    double odds = get_loan_payment_odds_percent(SSN, amount, daily_interest_rate, date) / 100.0;
    int successes = (int)(odds * N_SIMULATIONS);
    int failures = N_SIMULATIONS - successes;

    double gain = amount * (std::pow(1.0 + daily_interest_rate, 90) - 1.0);
    return (successes * gain) - (failures * amount);
}

double StochasticSimulator::get_best_interest_for_profit(
    size_t SSN, double amount, Date date)
{
    auto paths = stochastic_analysis(SSN, 1, 1, date);
    if (paths.empty()) return 0.0;

    int days = (int)paths[0].size();
    double best_rate = 0.0;
    double max_profit = -std::numeric_limits<double>::infinity();

    // Scan daily rates from 0.0% to 2.0%
    for (double rate = 0.0; rate <= 0.02; rate += 0.0005) {
        double required = amount * std::pow(1.0 + rate, days);
        int successes = 0;
        
        #pragma omp parallel for reduction(+:successes)
        for (int i = 0; i < (int)paths.size(); i++) {
            if (paths[i].back() >= required) successes++;
        }

        double gain = required - amount;
        double profit = (successes * gain) - ((N_SIMULATIONS - successes) * amount);

        if (profit > max_profit) {
            max_profit = profit;
            best_rate = rate;
        }
    }
    return best_rate;
}

// ─────────────────────────────────────────────────────────────────────
// Overloads and Helpers (Omitted for brevity, but map to logic above)
// ─────────────────────────────────────────────────────────────────────

std::vector<std::vector<double>> StochasticSimulator::stochastic_analysis(size_t SSN, Date pay_date) {
    size_t job = 1, loc = 1;
    try {
        job = DataBaseManager::get_current_job(SSN);
        loc = DataBaseManager::get_current_location(SSN);
    } catch (...) {}
    return stochastic_analysis(SSN, job, loc, pay_date);
}
