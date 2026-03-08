#include "stocastic_simulation.hpp"
#include "database_manager.hpp"
#include "datatypes.hpp"
#include <immintrin.h>

#include <iostream>
#include <random>
#include <chrono>
#include <thread>

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

    auto now      = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm* local = std::localtime(&t);

    Date curr_date{local->tm_mday, local->tm_mon + 1, local->tm_year + 1900};
    int m = pay_date - curr_date;

    std::cout << "  [sim] SSN=" << SSN << " days=" << m << " paths=" << N_SIMULATIONS << "\n";

    if (m <= 0) {
        std::cout << "  [sim] pay_date in the past, returning empty\n";
        return {};
    }

    std::vector<std::pair<double, double>> records(m);
    auto return_value = std::vector<std::vector<double>>(N_SIMULATIONS, std::vector<double>(m, 0.0));

    auto pay_days = DataBaseManager::get_pay_days({1, SSN, location, ocupation});

    auto t1 = std::chrono::high_resolution_clock::now();

    std::vector<DayCache> cache(m);

    // Step 1 — precompute everything per day index
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < m; i++) {
        Date target = curr_date + Date{i, 0, 0};
        auto data = MC{
            .day           = target.day,
            .month         = target.month,
            .year          = target.year,
            .day_of_week   = day_of_week(target),
            .location_id   = location,
            .profession_id = ocupation
        };
        auto [mean, sd] = predict_variation(data);
        double payday_injection = 0.0;
        auto it = pay_days->find(target);
        if (it != pay_days->end()) payday_injection = it->second;

        cache[i] = {mean, sd, payday_injection};
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "  [sim] Step 1 (prefetch) done in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
              << "ms\n";

    // Step 2 — pure arithmetic, zero date calls
    #pragma omp parallel
    {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count()
                        ^ (std::hash<std::thread::id>{}(std::this_thread::get_id()));
        std::default_random_engine generator(seed);
        std::normal_distribution<double> normal(0.0, 1.0);

        #pragma omp for schedule(static)
        for (int i = 0; i < (int)N_SIMULATIONS; i++) {
            for (int j = 0; j < m; j++) {
                return_value[i][j] = normal(generator) * cache[j].sd
                                    + cache[j].mean
                                    + cache[j].payday;
            }
        }
    }

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
    }

    return (100.0 * successes) / (double)paths.size();
}

double StochasticSimulator::get_estimated_loan_profit(
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
}
