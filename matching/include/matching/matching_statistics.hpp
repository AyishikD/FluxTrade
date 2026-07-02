#pragma once

#include <cstdint>

namespace fluxtrade {

struct MatchingStatistics {
    uint64_t orders_received = 0;
    uint64_t orders_matched = 0;
    uint64_t orders_cancelled = 0;
    uint64_t orders_rejected = 0;
    uint64_t trades_executed = 0;
    uint64_t shares_executed = 0;
    uint64_t total_traded_volume = 0;
    uint64_t total_latency_ns = 0;
    uint64_t max_latency_ns = 0;

    // Latency histogram buckets
    uint64_t bucket_under_100ns = 0;
    uint64_t bucket_100_250ns = 0;
    uint64_t bucket_250_500ns = 0;
    uint64_t bucket_500ns_1us = 0;
    uint64_t bucket_1us_5us = 0;
    uint64_t bucket_over_5us = 0;

    void record_latency(uint64_t ns) noexcept {
        total_latency_ns += ns;
        if (ns > max_latency_ns) {
            max_latency_ns = ns;
        }

        if (ns < 100) {
            bucket_under_100ns++;
        } else if (ns < 250) {
            bucket_100_250ns++;
        } else if (ns < 500) {
            bucket_250_500ns++;
        } else if (ns < 1000) {
            bucket_500ns_1us++;
        } else if (ns < 5000) {
            bucket_1us_5us++;
        } else {
            bucket_over_5us++;
        }
    }
};

} // namespace fluxtrade
