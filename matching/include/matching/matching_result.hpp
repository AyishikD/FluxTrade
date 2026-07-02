#pragma once

#include <cstdint>

namespace fluxtrade {

enum class MatchingStatus : uint8_t {
    Success         = 0,
    PartiallyFilled = 1,
    Filled          = 2,
    Rejected        = 3,
    Cancelled       = 4,
    Modified        = 5,
    Expired         = 6
};

struct MatchingResult {
    MatchingStatus status = MatchingStatus::Success;
    uint32_t trades_generated = 0;
};

} // namespace fluxtrade
