#pragma once

#include <domain/ids.hpp>
#include <domain/price.hpp>
#include <domain/quantity.hpp>
#include <domain/side.hpp>
#include <domain/order_type.hpp>
#include <domain/time_in_force.hpp>
#include <cstdint>

namespace fluxtrade {

enum class CommandType : uint8_t {
    NewOrder     = 0,
    CancelOrder  = 1,
    ReplaceOrder = 2
};

// Domain-level inbound request command representation
struct alignas(8) OrderCommand {
    CommandType type = CommandType::NewOrder;
    uint8_t padding[7] = {0, 0, 0, 0, 0, 0, 0};
    
    uint64_t sequence = 0;
    uint64_t timestamp = 0;
    uint64_t client_order_id = 0;
    uint64_t orig_client_order_id = 0;

    SymbolId symbol_id;
    AccountId account_id;
    Price price;
    Quantity qty;
    Side side = Side::Buy;
    OrderType order_type = OrderType::Limit;
    TimeInForce tif = TimeInForce::GTC;
    
    int32_t conn_id = -1;
};

} // namespace fluxtrade
