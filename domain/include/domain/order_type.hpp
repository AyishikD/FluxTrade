#pragma once

#include <cstdint>

namespace fluxtrade {

enum class OrderType : uint8_t {
    Limit  = 1,
    Market = 2,
    Stop   = 3
};

} // namespace fluxtrade
