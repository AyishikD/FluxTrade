#pragma once

#include <domain/ids.hpp>
#include <domain/price.hpp>
#include <domain/quantity.hpp>

namespace fluxtrade {

// Mutable Order View used inside the OrderBook matching structures (doubly-linked list node)
struct LiveOrder {
    OrderKey key;
    Price price;
    Quantity remaining_qty;
    uint64_t priority_seq = 0;
    
    // Intrusive doubly-linked list pointers for O(1) book updates/cancellations
    LiveOrder* next = nullptr;
    LiveOrder* prev = nullptr;
};

} // namespace fluxtrade
