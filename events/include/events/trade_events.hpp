#pragma once

#include <events/event_header.hpp>
#include <events/event_traits.hpp>
#include <common/types.hpp>

namespace fluxtrade {

struct alignas(8) TradeExecutedEvent {
    EventHeader header;
    OrderId maker_order_id = 0;
    OrderId taker_order_id = 0;
    Price price = 0;
    Qty qty = 0;
};

static_assert(sizeof(TradeExecutedEvent) == 72, "TradeExecutedEvent size must be 72 bytes");
static_assert(alignof(TradeExecutedEvent) == 8, "TradeExecutedEvent alignment must be 8 bytes");

struct alignas(8) TradeCancelledEvent {
    EventHeader header;
    OrderId maker_order_id = 0;
    OrderId taker_order_id = 0;
};

static_assert(sizeof(TradeCancelledEvent) == 56, "TradeCancelledEvent size must be 56 bytes");
static_assert(alignof(TradeCancelledEvent) == 8, "TradeCancelledEvent alignment must be 8 bytes");

// Trait Specializations
template <> struct is_trade_event<TradeExecutedEvent> : std::true_type {};
template <> struct is_trade_event<TradeCancelledEvent> : std::true_type {};

} // namespace fluxtrade
