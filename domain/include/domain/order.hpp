#pragma once

#include <domain/ids.hpp>
#include <domain/price.hpp>
#include <domain/quantity.hpp>
#include <domain/side.hpp>
#include <domain/order_type.hpp>
#include <domain/time_in_force.hpp>
#include <domain/order_flags.hpp>

namespace fluxtrade {

// Immutable Client Order Representation (64 Bytes, Fits exactly in one L1/L2 cache line)
struct alignas(8) Order {
    OrderId order_id;
    Price price;
    Quantity qty;
    uint64_t seq_num = 0;
    uint64_t client_timestamp = 0;
    uint64_t exchange_timestamp = 0;
    ClientId client_id;
    AccountId account_id;
    SymbolId symbol_id;
    Side side = Side::Buy;
    OrderType type = OrderType::Limit;
    TimeInForce tif = TimeInForce::GTC;
    OrderFlags flags = OrderFlags::None;
};

static_assert(sizeof(Order) == 64, "Order size must be exactly 64 bytes");
static_assert(alignof(Order) == 8, "Order alignment must be 8 bytes");

} // namespace fluxtrade
