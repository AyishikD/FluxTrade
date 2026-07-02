#pragma once

#include <events/event_header.hpp>
#include <events/event_traits.hpp>
#include <common/types.hpp>

namespace fluxtrade {

struct alignas(8) OrderReceivedEvent {
    EventHeader header;
    OrderId order_id = 0;
    Price price = 0;
    Qty qty = 0;
    ClientId client_id = 0;
    Side side = Side::Buy;
    uint8_t padding[3] = {0, 0, 0};
};

static_assert(sizeof(OrderReceivedEvent) == 72, "OrderReceivedEvent size must be 72 bytes");
static_assert(alignof(OrderReceivedEvent) == 8, "OrderReceivedEvent alignment must be 8 bytes");

struct alignas(8) OrderAcceptedEvent {
    EventHeader header;
    OrderId order_id = 0;
    Price price = 0;
    Qty qty = 0;
    ClientId client_id = 0;
    Side side = Side::Buy;
    uint8_t padding[3] = {0, 0, 0};
};

static_assert(sizeof(OrderAcceptedEvent) == 72, "OrderAcceptedEvent size must be 72 bytes");
static_assert(alignof(OrderAcceptedEvent) == 8, "OrderAcceptedEvent alignment must be 8 bytes");

struct alignas(8) OrderRejectedEvent {
    EventHeader header;
    OrderId order_id = 0;
    ClientId client_id = 0;
    uint32_t reject_reason_code = 0;
};

static_assert(sizeof(OrderRejectedEvent) == 56, "OrderRejectedEvent size must be 56 bytes");
static_assert(alignof(OrderRejectedEvent) == 8, "OrderRejectedEvent alignment must be 8 bytes");

struct alignas(8) OrderCancelledEvent {
    EventHeader header;
    OrderId order_id = 0;
    ClientId client_id = 0;
};

static_assert(sizeof(OrderCancelledEvent) == 56, "OrderCancelledEvent size must be 56 bytes");
static_assert(alignof(OrderCancelledEvent) == 8, "OrderCancelledEvent alignment must be 8 bytes");

// Trait Specializations
template <> struct is_order_event<OrderReceivedEvent> : std::true_type {};
template <> struct is_order_event<OrderAcceptedEvent> : std::true_type {};
template <> struct is_order_event<OrderRejectedEvent> : std::true_type {};
template <> struct is_order_event<OrderCancelledEvent> : std::true_type {};

} // namespace fluxtrade
