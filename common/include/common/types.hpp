#pragma once

#include <cstdint>

namespace fluxtrade {

using OrderId = uint64_t;
using Price = uint64_t;  // 4 decimal places implied (e.g. 10000 = 1.0000)
using Qty = uint64_t;
using ClientId = uint32_t;
using SymbolId = uint32_t;

enum class Side : uint8_t {
    Buy = 0,
    Sell = 1
};

} // namespace fluxtrade
