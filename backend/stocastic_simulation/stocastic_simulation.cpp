#include "stocastic_simulation.hpp"
#include "database_manager.hpp"
#include "datatypes.hpp"
#include <immintrin.h>

#include <iostream>
#include <random>
#include <chrono>
#include <thread>
<<<<<<< HEAD
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
=======

#include <omp.h> // Concurrent / Parallel programming

std::pair<double, double> StochasticSimulator::predict_variation(MC& m) {
    return {0.0, 1.0};
}

std::vector<std::vector<double>> StochasticSimulator::stochastic_analysis(size_t SSN, Date pay_date) {
    size_t job      = 1;
    size_t location = 1;
    try {
        job      = DataBaseManager::get_current_job(SSN);
        location = DataBaseManager::get_current_location(SSN);
    } catch (...) {}

    return stochastic_analysis(SSN, job, location, pay_date)  ;
}

std::vector<std::vector<double>> StochasticSimulator::stochastic_analysis(size_t SSN, size_t ocupation, size_t location, Date pay_date) {

>>>>>>> 263c7fa845312deb6526f9d8a432450becf9e19d
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

<<<<<<< HEAD
    auto return_value = std::vector<std::vector<double>>(
        N_SIMULATIONS, std::vector<double>(m, 0.0));
=======
    std::vector<std::pair<double, double>> records(m);
    auto return_value = std::vector<std::vector<double>>(N_SIMULATIONS, std::vector<double>(m, 0.0));

    auto pay_days = DataBaseManager::get_pay_days({1, SSN, location, ocupation});
>>>>>>> 263c7fa845312deb6526f9d8a432450becf9e19d

    DataRequest individual_req {0, SSN, location, ocupation};
    DataRequest class_req {0, 0, location, ocupation};

    auto individual_history = DataBaseManager::get_variance_history(individual_req);
    auto class_history      = DataBaseManager::get_variance_history(class_req);
    auto pay_days           = DataBaseManager::get_pay_days({ 1, SSN, location, ocupation });

    std::vector<DayCache> cache(m);
<<<<<<< HEAD
=======

    // Step 1 — precompute everything per day index
>>>>>>> 263c7fa845312deb6526f9d8a432450becf9e19d
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < m; i++) {
        Date target = curr_date + Date{ i, 0, 0 };
        auto data = MC{ target.day, target.month, target.year, day_of_week(target), location, ocupation, SSN };
        auto [mean, sd] = predict_variation(data, individual_history, class_history);
        
        double payday_injection = 0.0;
        auto it = pay_days->find(target);
        if (it != pay_days->end()) payday_injection = it->second;
<<<<<<< HEAD

        cache[i] = { mean, sd, payday_injection };
    }
=======

        cache[i] = {mean, sd, payday_injection};
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "  [sim] Step 1 (prefetch) done in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
              << "ms\n";
>>>>>>> 263c7fa845312deb6526f9d8a432450becf9e19d

    // Step 2 — pure arithmetic, zero date calls
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
<<<<<<< HEAD
                current_bal += (normal(generator) * cache[j].sd) + cache[j].mean + cache[j].payday;
                return_value[i][j] = current_bal;
=======
                return_value[i][j] = normal(generator) * cache[j].sd
                                    + cache[j].mean
                                    + cache[j].payday;
>>>>>>> 263c7fa845312deb6526f9d8a432450becf9e19d
            }
        }
    }

<<<<<<< HEAD
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
=======
    auto t3 = std::chrono::high_resolution_clock::now();
    std::cout << "  [sim] Step 2 (simulation) done in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count()
              << "ms\n";
    std::cout << "  [sim] Total: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t1).count()
              << "ms\n";

    return return_value;
}

std::vector<std::pair<double, double>> StochasticSimulator::stochastic_analysis_properties(size_t SSN, Date pay_date) {
    size_t job      = 1;
    size_t location = 1;
    try {
        job      = DataBaseManager::get_current_job(SSN);
        location = DataBaseManager::get_current_location(SSN);
    } catch (...) {}
    return stochastic_analysis_properties(SSN, job, location, pay_date);
}

std::vector<std::pair<double, double>> StochasticSimulator::stochastic_analysis_properties(size_t SSN, size_t ocupation, size_t location, Date pay_date) {
    std::vector<std::vector<double>> simulation = stochastic_analysis(SSN, ocupation, location, pay_date);

    if (simulation.empty()) return {};

    auto size   = simulation.size();
    auto length = simulation[0].size();
    auto result = std::vector<std::pair<double, double>>(size);

    // ── Pass 1: compute mean per path ─────────────────────────────────
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < (int)size; i++) {
        double avg = 0.0;
        for (int j = 0; j < (int)length; j++) {
            avg += simulation[i][j];
        }
        result[i].first = avg / length;
    }

    // ── Pass 2: compute SD per path ───────────────────────────────────
    // Bug fixed: original was missing the squared term
    // SD = sqrt( sum((x - mean)^2) / length )
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < (int)size; i++) {
        double variance = 0.0;
        for (int j = 0; j < (int)length; j++) {
            double diff = simulation[i][j] - result[i].first;
            variance += diff * diff;
        }
        result[i].second = std::sqrt(variance / length);
    }

    return result;
}

std::vector<bool> StochasticSimulator::evaluate_milestones(
    const std::vector<std::vector<double>>& paths,
    const std::vector<std::pair<_date, double>>& milestones,
    _date start_date,
    double interest_rate
) {
    std::vector<bool> results(paths.size(), false);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < (int)paths.size(); i++) {
        double balance       = 0.0;
        double rollover      = 0.0;
        size_t milestone_idx = 0;

        for (size_t day = 0; day < paths[i].size(); day++) {
            balance += paths[i][day];

            if (milestone_idx < milestones.size()) {
                int milestone_day = milestones[milestone_idx].first - start_date;

                if ((int)day == milestone_day) {
                    double required = milestones[milestone_idx].second + rollover;
                    milestone_idx++;

                    if (balance >= required) {
                        balance  -= required;
                        rollover  = 0.0;
                    } else if (balance > 0.0) {
                        rollover  = (required - balance) * (1.0 + interest_rate);
                        balance   = 0.0;
                    } else {
                        rollover  = required * (1.0 + interest_rate);
                    }
                }
            }
        }

        // Path succeeds if all milestones were reached and rollover is cleared
        results[i] = (milestone_idx == milestones.size() && rollover == 0.0);
    }

    return results;
}

std::vector<std::pair<_date, double>> StochasticSimulator::build_installment_schedule(
    double total_repayment,
    _date start_date,
    _date end_date,
    int num_payments
) {
    std::vector<std::pair<_date, double>> milestones;
    double payment_amount = total_repayment / num_payments;
    int total_days        = end_date - start_date;
    int interval          = total_days / num_payments;

    for (int p = 0; p < num_payments; p++) {
        _date payment_date = start_date + _date{interval * (p + 1), 0, 0};
        milestones.push_back({payment_date, payment_amount});
    }

    milestones.back().first = end_date;
    return milestones;
}

// ── shared helpers ────────────────────────────────────────────────────

static double compute_odds(const std::vector<bool>& results) {
    int successes = 0;
    #pragma omp parallel for reduction(+:successes)
    for (int i = 0; i < (int)results.size(); i++) {
        if (results[i]) successes++;
    }
    return (100.0 * successes) / (double)results.size();
}

static double compute_profit(
    const std::vector<bool>& results,
    double amount,
    double interest_rate
) {
    int successes = 0;
    #pragma omp parallel for reduction(+:successes)
    for (int i = 0; i < (int)results.size(); i++) {
        if (results[i]) successes++;
    }
    int failures = (int)results.size() - successes;
    return (successes * amount * interest_rate) + (failures * (-amount));
}

static _date infer_start_date(const std::vector<std::vector<double>>& paths, _date end_date) {
    return end_date + _date{-(int)paths[0].size(), 0, 0};
}

// ── single payment ────────────────────────────────────────────────────

double StochasticSimulator::get_loan_payment_odds_percent(
    size_t SSN, double amount, double interest_rate, Date date
) {
    auto paths = stochastic_analysis(SSN, date);
    if (paths.empty()) return 0.0;

    double required = amount * (1.0 + interest_rate);

    int successes = 0;
    #pragma omp parallel for reduction(+:successes) schedule(static)
    for (int i = 0; i < (int)paths.size(); i++) {
        double balance = 0.0;
        for (int j = 0; j < (int)paths[i].size(); j++)
            balance += paths[i][j];
        if (balance >= required) successes++;
>>>>>>> 263c7fa845312deb6526f9d8a432450becf9e19d
    }

    return (100.0 * successes) / (double)paths.size();
}

double StochasticSimulator::get_estimated_loan_profit(
<<<<<<< HEAD
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
=======
    size_t SSN, double amount, double interest_rate, Date date
) {
    auto paths = stochastic_analysis(SSN, date);
    if (paths.empty()) return 0.0;

    double required = amount * (1.0 + interest_rate);

    int successes = 0;
    #pragma omp parallel for reduction(+:successes) schedule(static)
    for (int i = 0; i < (int)paths.size(); i++) {
        double balance = 0.0;
        for (int j = 0; j < (int)paths[i].size(); j++)
            balance += paths[i][j];
        if (balance >= required) successes++;
    }

    int failures = (int)paths.size() - successes;
    return (successes * amount * interest_rate) + (failures * (-amount));
}

double StochasticSimulator::get_best_interest_for_profit(
    size_t SSN, double amount, Date date
) {
    auto paths = stochastic_analysis(SSN, date);
    if (paths.empty()) return 0.0;

    // Precompute final balance per path once
    std::vector<double> final_balances(paths.size(), 0.0);
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < (int)paths.size(); i++) {
        double balance = 0.0;
        for (int j = 0; j < (int)paths[i].size(); j++)
            balance += paths[i][j];
        final_balances[i] = balance;
    }

    double best_rate   = 0.0;
    double best_profit = std::numeric_limits<double>::lowest();

    for (double rate = 0.0; rate <= 1.0; rate += 0.005) {
        double required = amount * (1.0 + rate);

        int successes = 0;
        #pragma omp parallel for reduction(+:successes) schedule(static)
        for (int i = 0; i < (int)final_balances.size(); i++) {
            if (final_balances[i] >= required) successes++;
        }

        int failures  = (int)paths.size() - successes;
        double profit = (successes * amount * rate) + (failures * (-amount));

        if (profit > best_profit) {
            best_profit = profit;
            best_rate   = rate;
        }
    }

    return best_rate;
}

double StochasticSimulator::get_minimum_interest_for_profit(
    size_t SSN, double amount, std::vector<double> interest_rates, Date date
) {
    auto paths = stochastic_analysis(SSN, date);
    if (paths.empty()) return -1.0;

    // Precompute final balance per path once
    std::vector<double> final_balances(paths.size(), 0.0);
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < (int)paths.size(); i++) {
        double balance = 0.0;
        for (int j = 0; j < (int)paths[i].size(); j++)
            balance += paths[i][j];
        final_balances[i] = balance;
    }

    for (double rate : interest_rates) {
        double required = amount * (1.0 + rate);

        int successes = 0;
        #pragma omp parallel for reduction(+:successes) schedule(static)
        for (int i = 0; i < (int)final_balances.size(); i++) {
            if (final_balances[i] >= required) successes++;
        }

        int failures  = (int)paths.size() - successes;
        double profit = (successes * amount * rate) + (failures * (-amount));

        if (profit > 0.0) return rate;
    }

    return -1;
}

// ── convenience ───────────────────────────────────────────────────────

// Computes cumulative balance at each milestone day for every path
// Returns matrix: cumulative_balances[path][milestone]
static std::vector<std::vector<double>> compute_cumulative_at_milestones(
    const std::vector<std::vector<double>>& paths,
    const std::vector<std::pair<Date, double>>& milestones,
    Date start_date
) {
    std::vector<std::vector<double>> cumulative(paths.size(), std::vector<double>(milestones.size(), 0.0));

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < (int)paths.size(); i++) {
        double balance    = 0.0;
        size_t m_idx      = 0;
        int    next_day   = milestones[0].first - start_date;

        for (int j = 0; j < (int)paths[i].size() && m_idx < milestones.size(); j++) {
            balance += paths[i][j];
            if (j == next_day) {
                cumulative[i][m_idx] = balance;
                m_idx++;
                if (m_idx < milestones.size())
                    next_day = milestones[m_idx].first - start_date;
            }
        }
    }

    return cumulative;
}

// ── schedule odds ─────────────────────────────────────────────────────
std::vector<double> StochasticSimulator::get_payment_schedule_odds(
    size_t SSN, double amount, double interest_rate,
    Date start_date, Date end_date, int num_payments
) {
    auto milestones = build_installment_schedule(amount * (1.0 + interest_rate), start_date, end_date, num_payments);
    auto paths      = stochastic_analysis(SSN, end_date);
    if (paths.empty()) return {};

    auto cumulative = compute_cumulative_at_milestones(paths, milestones, start_date);
    std::vector<double> odds(milestones.size(), 0.0);

    for (int m = 0; m < (int)milestones.size(); m++) {
        double required = milestones[m].second;
        int successes   = 0;
        #pragma omp parallel for reduction(+:successes) schedule(static)
        for (int i = 0; i < (int)paths.size(); i++) {
            if (cumulative[i][m] >= required) successes++;
        }
        odds[m] = (100.0 * successes) / (double)paths.size();
    }

    return odds;
}

// ── explicit milestone overloads ──────────────────────────────────────
double StochasticSimulator::get_loan_payment_odds_percent(
    size_t SSN, double amount, double interest_rate,
    const std::vector<std::pair<Date, double>>& milestones, Date start_date
) {
    auto paths = stochastic_analysis(SSN, milestones.back().first);
    if (paths.empty()) return 0.0;

    auto cumulative = compute_cumulative_at_milestones(paths, milestones, start_date);

    // A path succeeds only if it meets every milestone
    int successes = 0;
    #pragma omp parallel for reduction(+:successes) schedule(static)
    for (int i = 0; i < (int)paths.size(); i++) {
        bool ok = true;
        for (int m = 0; m < (int)milestones.size(); m++) {
            if (cumulative[i][m] < milestones[m].second) { ok = false; break; }
        }
        if (ok) successes++;
    }

    return (100.0 * successes) / (double)paths.size();
}

double StochasticSimulator::get_estimated_loan_profit(
    size_t SSN, double amount, double interest_rate,
    const std::vector<std::pair<Date, double>>& milestones, Date start_date
) {
    auto paths = stochastic_analysis(SSN, milestones.back().first);
    if (paths.empty()) return 0.0;

    auto cumulative = compute_cumulative_at_milestones(paths, milestones, start_date);

    int successes = 0;
    #pragma omp parallel for reduction(+:successes) schedule(static)
    for (int i = 0; i < (int)paths.size(); i++) {
        bool ok = true;
        for (int m = 0; m < (int)milestones.size(); m++) {
            if (cumulative[i][m] < milestones[m].second) { ok = false; break; }
        }
        if (ok) successes++;
    }

    int failures = (int)paths.size() - successes;
    return (successes * amount * interest_rate) + (failures * (-amount));
}

double StochasticSimulator::get_best_interest_for_profit(
    size_t SSN, double amount,
    const std::vector<std::pair<Date, double>>& milestones, Date start_date
) {
    auto paths = stochastic_analysis(SSN, milestones.back().first);
    if (paths.empty()) return 0.0;

    // Compute cumulative balances once with dummy amounts — amounts don't affect balances
    auto cumulative = compute_cumulative_at_milestones(paths, milestones, start_date);
    int n_paths      = (int)paths.size();
    int n_milestones = (int)milestones.size();

    double best_rate   = 0.0;
    double best_profit = std::numeric_limits<double>::lowest();

    for (double rate = 0.0; rate <= 1.0; rate += 0.005) {
        double per_payment = (amount * (1.0 + rate)) / n_milestones;

        int successes = 0;
        #pragma omp parallel for reduction(+:successes) schedule(static)
        for (int i = 0; i < n_paths; i++) {
            bool ok = true;
            for (int m = 0; m < n_milestones; m++) {
                if (cumulative[i][m] < per_payment * (m + 1)) { ok = false; break; }
            }
            if (ok) successes++;
        }

        int failures  = n_paths - successes;
        double profit = (successes * amount * rate) + (failures * (-amount));
        if (profit > best_profit) {
            best_profit = profit;
            best_rate   = rate;
        }
    }

    return best_rate;
}

double StochasticSimulator::get_minimum_interest_for_profit(
    size_t SSN, double amount, std::vector<double> interest_rates,
    const std::vector<std::pair<Date, double>>& milestones, Date start_date
) {
    auto paths = stochastic_analysis(SSN, milestones.back().first);
    if (paths.empty()) return -1.0;

    auto cumulative  = compute_cumulative_at_milestones(paths, milestones, start_date);
    int n_paths      = (int)paths.size();
    int n_milestones = (int)milestones.size();

    for (double rate : interest_rates) {
        double per_payment = (amount * (1.0 + rate)) / n_milestones;

        int successes = 0;
        #pragma omp parallel for reduction(+:successes) schedule(static)
        for (int i = 0; i < n_paths; i++) {
            bool ok = true;
            for (int m = 0; m < n_milestones; m++) {
                if (cumulative[i][m] < per_payment * (m + 1)) { ok = false; break; }
            }
            if (ok) successes++;
        }

        int failures  = n_paths - successes;
        double profit = (successes * amount * rate) + (failures * (-amount));
        if (profit > 0.0) return rate;
    }

    return -1.0;
}

// ── convenience overloads ─────────────────────────────────────────────
double StochasticSimulator::get_loan_payment_odds_percent(
    size_t SSN, double amount, double interest_rate,
    Date start_date, Date end_date, int num_payments
) {
    auto milestones = build_installment_schedule(amount * (1.0 + interest_rate), start_date, end_date, num_payments);
    return get_loan_payment_odds_percent(SSN, amount, interest_rate, milestones, start_date);
}

double StochasticSimulator::get_estimated_loan_profit(
    size_t SSN, double amount, double interest_rate,
    Date start_date, Date end_date, int num_payments
) {
    auto milestones = build_installment_schedule(amount * (1.0 + interest_rate), start_date, end_date, num_payments);
    return get_estimated_loan_profit(SSN, amount, interest_rate, milestones, start_date);
}

double StochasticSimulator::get_best_interest_for_profit(
    size_t SSN, double amount,
    Date start_date, Date end_date, int num_payments
) {
    auto milestones = build_installment_schedule(amount, start_date, end_date, num_payments);
    return get_best_interest_for_profit(SSN, amount, milestones, start_date);
}

double StochasticSimulator::get_minimum_interest_for_profit(
    size_t SSN, double amount, std::vector<double> interest_rates,
    Date start_date, Date end_date, int num_payments
) {
    auto milestones = build_installment_schedule(amount, start_date, end_date, num_payments);
    return get_minimum_interest_for_profit(SSN, amount, interest_rates, milestones, start_date);
>>>>>>> 263c7fa845312deb6526f9d8a432450becf9e19d
}
