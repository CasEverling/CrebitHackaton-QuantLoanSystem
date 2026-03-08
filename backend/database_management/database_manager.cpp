
#include "database_manager.hpp"
#include <memory>

void DatabaseManager::createDatabase() {
    pqxx::connection c("dbname=postgres user=postgres password=sua_nova_senha host=127.0.0.1 port=5432");
    pqxx::work txn(c);
    
    txn.exec("CREATE DATABASE loan_database");

    // Table 1 - User
    txn.exec(R"(
        CREATE TABLE IF NOT EXISTS Users(
            SSN BIGINT PRIMARY KEY NOT NULL,
            USERNAME TEXT NOT NULL,
            PASSWORD TEXT NOT NULL
        )
    )");

    // Table 2 - FixedIncome
    txn.exec(R"(
        CREATE TABLE IF NOT EXISTS FixedIncome(
            ID SERIAL PRIMARY KEY,
            SSN BIGINT NOT NULL REFERENCES Users(SSN),
            PAY_DAY INT NOT NULL,
            AMOUNT DECIMAL(15,2) NOT NULL,
            START_DAY INT NOT NULL,
            START_MONTH INT NOT NULL,
            START_YEAR INT NOT NULL,
            END_DAY INT NOT NULL,
            END_MONTH INT NOT NULL,
            END_YEAR INT NOT NULL
        )
    )");

    // Table 5 - FreelancingIncome
    txn.exec(R"(
        CREATE TABLE IF NOT EXISTS FreelancingIncome(
            ID SERIAL PRIMARY KEY,
            SSN BIGINT NOT NULL REFERENCES Users(SSN),
            ACTIVITY_TYPE INT NOT NULL,
            START_DAY INT NOT NULL,
            START_MONTH INT NOT NULL,
            START_YEAR INT NOT NULL,
            END_DAY INT NOT NULL,
            END_MONTH INT NOT NULL,
            END_YEAR INT NOT NULL
        )
    )");

    // Table 6 - Location
    txn.exec(R"(
        CREATE TABLE IF NOT EXISTS Location(
            ID SERIAL PRIMARY KEY,
            SSN BIGINT NOT NULL REFERENCES Users(SSN),
            LOCATION_ID INT NOT NULL,
            MOVE_IN_DAY INT NOT NULL,
            MOVE_IN_MONTH INT NOT NULL,
            MOVE_IN_YEAR INT NOT NULL
        )
    )");

    // Table 7 - AccountBalance
    txn.exec(R"(
        CREATE TABLE IF NOT EXISTS AccountBalance(
            ID SERIAL PRIMARY KEY,
            SSN BIGINT NOT NULL REFERENCES Users(SSN),
            BALANCE DECIMAL(15,2) NOT NULL,
            DAY INT NOT NULL,
            WEEK INT NOT NULL,
            MONTH INT NOT NULL,
            YEAR INT NOT NULL,
            DAY_OF_WEEK INT NOT NULL CHECK (DAY_OF_WEEK BETWEEN 0 AND 6),
            BALANCE_REASON TEXT NOT NULL CHECK (BALANCE_REASON IN ('fixed', 'volatile'))
        )
    )");

    // Table 8 - AccountBalanceVariableVariation
    txn.exec(R"(
        CREATE TABLE IF NOT EXISTS AccountBalanceVariableVariation(
            ID SERIAL PRIMARY KEY,
            SSN BIGINT NOT NULL REFERENCES Users(SSN),
            BALANCE DECIMAL(15,2) NOT NULL,
            DAY INT NOT NULL,
            WEEK INT NOT NULL,
            MONTH INT NOT NULL,
            YEAR INT NOT NULL,
            DAY_OF_WEEK INT NOT NULL CHECK (DAY_OF_WEEK BETWEEN 0 AND 6),
            BALANCE_REASON TEXT NOT NULL
        )
    )");

    txn.commit();
}

std::shared_ptr<std::vector<double>> 
DataBaseManager::get_variance_history(const DataRequest& data) {
    if (cache.find(data) != cache.end())
        if (auto sharedFromWeak = weak.lock())
            return std::move(sharedFromWeak);
            
    // Perform query
    // Create Vector
    // Return pointer to vector
}