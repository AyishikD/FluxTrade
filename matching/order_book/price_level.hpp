#pragma once

#include <domain/price.hpp>
#include <domain/quantity.hpp>
#include <matching/order_book/intrusive_list.hpp>

namespace fluxtrade {

struct PriceLevelNode {
    Price price;
    Quantity total_qty;
    uint32_t order_count = 0;
    
    IntrusiveOrderList orders;
    
    PriceLevelNode* next = nullptr; // Next level (Bid down, Ask up)
    PriceLevelNode* prev = nullptr; // Previous level (Bid up, Ask down)
    
    [[nodiscard]] bool empty() const noexcept { return orders.empty(); }
    [[nodiscard]] bool has_orders() const noexcept { return !orders.empty(); }
};

} // namespace fluxtrade
