#pragma once

#include <cstdint>

namespace fluxtrade {

enum class OrderStatus : uint8_t {
    New             = 1,
    PartiallyFilled = 2,
    Filled          = 3,
    Cancelled       = 4,
    Rejected        = 5
};

} // namespace fluxtrade
