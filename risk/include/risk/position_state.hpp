#pragma once

#include <cstdint>

namespace fluxtrade {

// Symbol position limits per Account (Exactly 64 Bytes, Fits inside a single CPU cache line)
struct alignas(8) PositionState {
    int64_t net_qty = 0;      // Positive = Long, Negative = Short
    uint64_t long_qty = 0;
    uint64_t short_qty = 0;
    uint64_t gross_qty = 0;   // Long + Short
    
    // Limits
    uint64_t max_long_limit = 0;
    uint64_t max_short_limit = 0;
    
    // Context identifiers (4 bytes each)
    uint32_t symbol_id = 0;
    uint32_t account_id = 0;

    // Alignment padding (8 bytes total)
    uint64_t last_update_ts = 0;
};

static_assert(sizeof(PositionState) == 64, "PositionState size must be exactly 64 bytes");
static_assert(alignof(PositionState) == 8, "PositionState alignment must be 8 bytes");

} // namespace fluxtrade
