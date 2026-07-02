#pragma once

#include <cstdint>

namespace fluxtrade {

struct RiskStatistics {
    uint64_t accepted_count = 0;
    uint64_t rejected_count = 0;
    
    // Pipeline stage-specific reject counts
    uint64_t credit_rejects = 0;
    uint64_t position_rejects = 0;
    uint64_t price_band_rejects = 0;
    uint64_t fat_finger_rejects = 0;
    uint64_t stp_rejects = 0;
    uint64_t kill_switch_rejects = 0;
    uint64_t duplicate_rejects = 0;
    uint64_t rate_limit_rejects = 0;
    uint64_t circuit_breaker_rejects = 0;

    uint64_t total_latency_ns = 0;
    uint64_t max_latency_ns = 0;

    void record_latency(uint64_t ns) noexcept {
        total_latency_ns += ns;
        if (ns > max_latency_ns) {
            max_latency_ns = ns;
        }
    }
};

} // namespace fluxtrade
