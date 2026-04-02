#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <functional>
#include <ctime>


struct _market_context {
    int day;
    int month;
    int year;
    int day_of_week;
    size_t location_id;
    size_t profession_id;
    size_t ssn;           // added: needed by predict_variation for individual history

    bool operator==(const _market_context& other) const {
        return day           == other.day           &&
               month         == other.month         &&
               year          == other.year          &&
               day_of_week   == other.day_of_week   &&
               location_id   == other.location_id   &&
               profession_id == other.profession_id &&
               ssn           == other.ssn;
    }
};

namespace std {
    template <>
    struct hash<_market_context> {
        size_t operator()(const _market_context& mc) const {
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
            hash_combine(h, hash<size_t>{}(mc.ssn));   // added
            return h;
        }
    };
}

struct _date {
    int day;
    int month;
    int year;

    // ── prefix ++ ──────────────────────────────────────────────────────
    _date& operator++() {
        std::tm t{};
        t.tm_mday = day;
        t.tm_mon  = month - 1;
        t.tm_year = year  - 1900;
        std::mktime(&t);
        t.tm_mday++;
        std::mktime(&t);
        day   = t.tm_mday;
        month = t.tm_mon  + 1;
        year  = t.tm_year + 1900;
        return *this;
    }

    // ── postfix ++ ─────────────────────────────────────────────────────
    _date operator++(int) {
        _date old = *this;
        ++(*this);
        return old;
    }

    // ── equality ───────────────────────────────────────────────────────
    bool operator==(const _date& o) const {
        return day == o.day && month == o.month && year == o.year;
    }

    // ── less-than-or-equal ─────────────────────────────────────────────
    bool operator<=(const _date& o) const {
        if (year  != o.year)  return year  < o.year;
        if (month != o.month) return month < o.month;
        return day <= o.day;
    }

    // ── difference in days (a - b) ─────────────────────────────────────
    int operator-(const _date& b) const {
        std::tm ta{}, tb{};
        ta.tm_mday = day;   ta.tm_mon = month - 1;   ta.tm_year = year  - 1900;
        tb.tm_mday = b.day; tb.tm_mon = b.month - 1; tb.tm_year = b.year - 1900;
        std::time_t ta_t = std::mktime(&ta);
        std::time_t tb_t = std::mktime(&tb);
        return static_cast<int>(std::difftime(ta_t, tb_t) / 86400.0);
    }

    // ── addition: shift a date by a {days, months, years} delta ────────
    _date operator+(const _date& delta) const {
        std::tm t{};
        t.tm_mday = day   + delta.day;
        t.tm_mon  = month - 1 + delta.month;
        t.tm_year = year  - 1900 + delta.year;
        std::mktime(&t);
        return _date{t.tm_mday, t.tm_mon + 1, t.tm_year + 1900};
    }
};

// ── day of week (free function) ────────────────────────────────────────
// Returns 0 = Sunday … 6 = Saturday  (matches tm_wday)
inline int day_of_week(const _date& d) {
    std::tm t{};
    t.tm_mday = d.day;
    t.tm_mon  = d.month - 1;
    t.tm_year = d.year  - 1900;
    std::mktime(&t);
    return t.tm_wday;
}

struct _data_request {
    size_t resolution;
    size_t ssn;        // 0 = class-level query
    size_t location;
    size_t job;
};


using DataRequest  = struct _data_request;
using MB           = struct _market_behavior;
using MC           = struct _market_context;
using variation_dp = std::unordered_map<MC, std::pair<double,double>>;
using Date         = struct _date;

// ── operator== for unordered_map keys ────────────────────────────────

inline bool operator==(const _data_request& a, const _data_request& b) {
    return a.resolution == b.resolution &&
           a.ssn        == b.ssn        &&
           a.location   == b.location   &&
           a.job        == b.job;
}

// ── std::hash specializations ─────────────────────────────────────────

namespace std {
    template<> struct hash<_date> {
        size_t operator()(const _date& d) const noexcept {
            size_t h = d.year * 10000 + d.month * 100 + d.day;
            return hash<size_t>{}(h);
        }
    };

    template<> struct hash<_data_request> {
        size_t operator()(const _data_request& r) const noexcept {
            size_t h = r.resolution;
            h ^= hash<size_t>{}(r.ssn)      + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= hash<size_t>{}(r.location) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= hash<size_t>{}(r.job)      + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };
}
