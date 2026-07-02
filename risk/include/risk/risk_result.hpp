#pragma once

#include <domain/reject_reason.hpp>
#include <cstdint>

namespace fluxtrade {

enum class RiskDecision : uint8_t {
    Accept  = 0,
    Reject  = 1,
    Hold    = 2,
    Auction = 3,
    Halt    = 4
};

// Packed Risk Result (Exactly 32 Bytes, 8-Byte Aligned)
struct alignas(8) RiskResult {
    // 2-byte + 1-byte + 5-byte padding = exactly 8 bytes
    RejectReason reason = RejectReason::None;
    RiskDecision decision = RiskDecision::Accept;
    uint8_t padding[5] = {0, 0, 0, 0, 0};
    
    // 8-byte members (24 bytes)
    uint64_t timestamp = 0;
    uint64_t reserve_capital = 0;
    uint64_t pipeline_latency_ns = 0;
};

static_assert(sizeof(RiskResult) == 32, "RiskResult size must be exactly 32 bytes");
static_assert(alignof(RiskResult) == 8, "RiskResult alignment must be 8 bytes");

} // namespace fluxtrade
