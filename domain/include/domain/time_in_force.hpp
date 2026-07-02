#pragma once

#include <cstdint>

namespace fluxtrade {

enum class TimeInForce : uint8_t {
    GTC = 1, // Good Till Cancelled
    IOC = 2, // Immediate Or Cancel
    FOK = 3  // Fill Or Kill
};

} // namespace fluxtrade
