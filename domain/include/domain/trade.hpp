#pragma once

#include <domain/ids.hpp>
#include <domain/price.hpp>
#include <domain/quantity.hpp>

namespace fluxtrade {

struct alignas(8) Trade {
    TradeId trade_id;
    OrderId buy_order_id;
    OrderId sell_order_id;
    Price price;
    Quantity qty;
    uint64_t timestamp = 0;
    SymbolId symbol_id;
    uint32_t padding = 0; // Ensures 8-byte padding boundary
};

static_assert(sizeof(Trade) == 56, "Trade size must be exactly 56 bytes");
static_assert(alignof(Trade) == 8, "Trade alignment must be 8 bytes");

} // namespace fluxtrade
