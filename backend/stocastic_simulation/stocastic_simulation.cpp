#include "stocastic_simulation.hpp"
#include "./database_management/database_manager.hpp"
#include "./datatypes.hpp"
#include <immintrin.h>

#include <iostream>
#include <random>
#include <chrono>

#include <omp.h>

std::pair<double, double> StochasticSimulator::predict_variation(MC& m) {
    if (records.find(m) != records.end())
        return records[m];
    return {0.0, 1.0};
}

std::vector<std::vector<double>> StochasticSimulator::stochastic_analysis(size_t SSN, Date max_date) {
    auto job = DataBaseManager::get_current_job(SSN);
    auto location = DataBaseManager::get_current_location(SSN);

    return stochastic_analysis(SSN, job, location, max_date)  ;
}

std::vector<std::vector<double>> StochasticSimulator::stochastic_analysis(size_t SSN, size_t ocupation, size_t location, Date max_date) {
    // Implementação da análise estocástica para um SSN específico, ocupação e localização
    // Esta função deve acessar os dados do banco de dados e realizar a simulação

    auto initial_money = DataBaseManager::get_variance_history(
        {1, SSN, location, ocupation}
    )[0];

    auto return_value = std::vector<std::vector<double>>(
        1000, std::vector<double>(200, initial_money)
    );

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::atomic<std::normal_distribution<double>> standard_normal_distribution(0, 1); 
    
    auto pay_days = DataBaseManager::get_pay_days({1, SSN, location, ocupation});

    max_iterations = 0;

    #pragma omp parallel 
    {
        bool is_main_thread = omp_get_thread_num() == 0;
        int curr_iteration = 0;
        for (size_t i = 0; i < return_value.size(); ++i) {
            for (size_t j = 1; j < return_value[i].size(); ++j) {
                auto data = MC{
                    .day = j % 30 + 1,
                    .month = (j / 30) % 12 + 1,
                    .year = 2020 + (j / 360),
                    .day_of_week = j % 7,
                    .location_id = location,
                    .profession_id = ocupation
                };
                
                if (curr_iteration >= max_iterations.load(std::memory_order_acquire)) {
                    if (!is_main_thread) {
                        do {
                            _mm_stop(); // Sipins until other threas does its work
                        } while (curr_iteration >= max_iterations.load(std::memory_order_acquire));

                        return_value[i][j] = 
                            standard_normal_distribution() * records[data].second + 
                            records[data].first + 
                            pay_days[Date{.day = data.day, .month = data.month, .year = data.year}];
                    }
                    else {
                        records[data] = predict_variation(data);
                        // Aditio0n is replicated to ensure that the main thread does its wotk before other
                        // Reduced odds of other threads catching up to the main thread
                        // Avoid unnecessary spins with _mm_pause()
                        return_value[i][j] =
                            standard_normal_distribution() * records[data].second + 
                            records[data].first + 
                            pay_days[Date{.day = data.day, .month = data.month, .year = data.year}];
                        max_iterations.fetch_add(1, std::memory_order_release);
                    }
                }
            }
        }
    }
    

    return std::move(return_value);
}