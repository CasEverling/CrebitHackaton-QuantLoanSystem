#include <vector>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <functional>

struct _market_context {
    int day;
    int month;
    int year;
    int day_of_week;
    size_t location_id;
    size_t profession_id;

    bool operator==(const _market_context& other) const {
        return day == other.day &&
               month == other.month &&
               year == other.year &&
               day_of_week == other.day_of_week &&
               location_id == other.location_id &&
               profession_id == other.profession_id;
    }
};


struct _date {
    int day;
    int month;
    int year;
};

namespace std {
    template <>
    struct hash<_market_context> {
        size_t operator()(const _market_context& mc) const {
            // Função auxiliar simples para combinar hashes
            auto hash_combine = [](size_t& seed, size_t v) {
                seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            };

            size_t h = 0;
            hash_combine(h, hash<int>{}(mc.day));
            hash_combine(h, hash<int>{}(mc.month));
            hash_combine(h, hash<int>{}(mc.year));
            hash_combine(h, hash<int>{}(mc.day_of_week));
            hash_combine(h, hash<size_t>{}(mc.location_id));
            hash_combine(h, hash<size_t>{}(mc.profession_id));
            
            return h;
        }
    };
}


struct _data_request {
    size_t resolution;
    size_t ssn; // 0 refers to any
    size_t location;
    size_t job;
};

namespace std {
    template <>
    struct hash<_data_request> {
        size_t operator()(const _data_request& dr) const {
            // Função auxiliar simples para combinar hashes
            auto hash_combine = [](size_t& seed, size_t v) {
                seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            };

            size_t h = 0;
            hash_combine(h, hash<size_t>{}(dr.resolution));
            hash_combine(h, hash<size_t>{}(dr.ssn));
            hash_combine(h, hash<size_t>{}(dr.location));
            hash_combine(h, hash<size_t>{}(dr.job));
            
            return h;
        }
    };
}

using DataRequest = struct _data_request;
using MB = struct _market_behavior;
using MC = struct _market_context;
using variation_dp = std::unordered_map<MC, std::pair<double,double>>;
using Date = struct _date;
