#pragma once

#include <cstdint>

namespace fluxtrade {

enum class RejectReason : uint16_t {
    None               = 0,
    InvalidPrice       = 1,
    InvalidQuantity    = 2,
    InvalidSymbol      = 3,
    DuplicateOrder     = 4,
    OrderNotFound      = 5,
    RiskLimitExceeded  = 6
};

} // namespace fluxtrade
