#include <vector>
#include <unordered_map>
#include <memory>
#include "../backend/datatypes.hpp"

/*
    TABLE 1 - User
    | SSN | Username | Password

    TABLE 2 - FixedIncome
    | SSN | PayDay | Amount | StartDate | EndDate

    TABLE 5 - FreelancingIncome
    | SSN | ActivityType | StartDate | EndDate

    TABLE 6 - Location
    | SSN | location | MoveInDate

    TABLE 7 - AccountBalance
    | SSN | Balance | Day | Week | Year | DayOfWeek | BalanceReason (fixed or volatile)

    TABLE 8 - AccountBalanceVariableVariation - we exclude the fixed income with is always add on the paydays in the simulation
    | SSN | Balance | Day | Week | Year | DayOfWeek | BalanceReadon

*/

class DataBaseManager {
    private:
        static void createDatabase();
        static void loadDatabase();

        static std::unordered_map<DataRequest, std::weak_ptr<double>> cache;
        //TODO: create all objects to hold pointers
        
    public:
        static std::shared_ptr<std::vector<double>> get_variance_history(const DataRequest& data);
        static std::shared_ptr<std::unordered_map<Date, double>> get_pay_days(const DataRequest& data);
        static size_t get_current_job(size_t SSN);
        static size_t get_current_location(size_t SSN);
}