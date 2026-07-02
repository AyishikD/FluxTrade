#pragma once

#include <domain/ids.hpp>
#include <domain/price.hpp>
#include <domain/quantity.hpp>

namespace fluxtrade {

struct alignas(8) Fill {
    FillId fill_id;
    TradeId trade_id;
    OrderId order_id;
    Price price;
    Quantity qty;
    uint64_t timestamp = 0;
};

static_assert(sizeof(Fill) == 48, "Fill size must be exactly 48 bytes");
static_assert(alignof(Fill) == 8, "Fill alignment must be 8 bytes");

} // namespace fluxtrade
