#include "stocastic_simulation.hpp"
#include "./database_management/database_manager.hpp"
#include "./datatypes.hpp"
#include <immintrin.h>

#include <iostream>
#include <random>
#include <chrono>

#include <Eigen/Dense> // Matirx Operations
#include <omp.h> // Concurrent / Parallel programming

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

std::vector<std::vector<double>> StochasticSimulator::stochastic_analysis(size_t SSN, size_t ocupation, size_t location, Date pay_date) {
    // Implementação da análise estocástica para um SSN específico, ocupação e localização
    // Esta função deve acessar os dados do banco de dados e realizar a simulação

    // Pipeline step 0.1 getting current amount of money
    auto initial_money = DataBaseManager::get_variance_history(
        {1, SSN, location, ocupation}
    )[0];

    
    // Pipeline step 0.2 setting date properties
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm* local = std::localtime(&t);

    auto curr_date = Date{
        local->tm_mday,
        local->tm_mon + 1,
        local->tm_year + 1900
    };
    int curr_day_of_week = day_of_week(curr_date);
    auto m = pay_date - curr_date;

    auto return_value = std::vector<std::vector<double>>(
        1000, std::vector<double>(m, initial_money)
    );
    
    // Pipeline step 0.3 setting random properties
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::atomic<std::normal_distribution<double>> standard_normal_distribution(0, 1); 
    
    // Pipeline step 0.4 Considering Payday -- If key does not exist returns 0
    auto pay_days = DataBaseManager::get_pay_days({1, SSN, location, ocupation});
    

    // Pipeline Step 1 - Get statistical Distributions
    max_iterations = 0;

    #pragma omp parallel for
    for (int i = 0; i < m; i++) {
        auto data = MC{
                    .day = i % 30 + 1,
                    .month = (i / 30) % 12 + 1,
                    .year = 2020 + (j / 360),
                    .day_of_week = i % 7,
                    .location_id = location,
                    .profession_id = ocupation
                };
        records[data] = predict_variation(data);
    }

    // Pipeline Step 2 - Simulate M different paths
    #pragma omp parallel 
    {
        bool is_main_thread = omp_get_thread_num() == 0;
        int curr_iteration = 0;

        #pragma omp for
        for (size_t i = 0; i < N_SIMULATIONS; ++i) {
            auto date = curr_date;
            for (aint j = 0 j < M; ++j) {
                auto data = MC{
                    .day = date.day,
                    .month = date.month,
                    .year = date.year,
                    .day_of_week = ,
                    .location_id = location,
                    .profession_id = ocupation
                };
                
            
                return_value[i][j] = 
                    standard_normal_distribution() * records[data].second + 
                    records[data].first + 
                    pay_days[Date{.day = data.day, .month = data.month, .year = data.year}];
                
                date++;

            }
        }
    }

    return std::move(return_value);
}


