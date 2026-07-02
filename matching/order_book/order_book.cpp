#include <matching/order_book/order_book.hpp>
#include <cassert>

namespace fluxtrade {

LimitOrderBook::LimitOrderBook(IOrderBookListener* listener) noexcept
    : listener_(listener) {}

LiveOrder* LimitOrderBook::insert(const Order& order, uint64_t priority_seq) noexcept {
    // 1. Allocate LiveOrder from pool
    LiveOrder* live_order = order_pool_.allocate();
    live_order->key = OrderKey{order.symbol_id, order.order_id};
    live_order->price = order.price;
    live_order->remaining_qty = order.qty;
    live_order->priority_seq = priority_seq;
    live_order->next = nullptr;
    live_order->prev = nullptr;

    // 2. Index in lookup map
    lookup_.insert(order.order_id, live_order);

    // 3. Locate or create price level
    PriceLevelNode* level = nullptr;
    if (order.side == Side::Buy) {
        level = bids_.find_level(order.price);
        if (!level) {
            level = bids_.create_level(order.price, level_pool_);
            if (listener_) {
                listener_->on_price_level_created(order.price, Side::Buy);
            }
        }
    } else {
        level = asks_.find_level(order.price);
        if (!level) {
            level = asks_.create_level(order.price, level_pool_);
            if (listener_) {
                listener_->on_price_level_created(order.price, Side::Sell);
            }
        }
    }

    // 4. Append to intrusive list
    level->orders.append(live_order);
    level->total_qty += order.qty;
    level->order_count++;

    // 5. Notify listener
    if (listener_) {
        listener_->on_order_inserted(*live_order);
    }

    return live_order;
}

bool LimitOrderBook::cancel(const OrderId& order_id) noexcept {
    LiveOrder* order = lookup_.find(order_id);
    if (!order) return false;

    // Determine side and price level (prices are unique across bids and asks)
    PriceLevelNode* level = bids_.find_level(order->price);
    Side side = Side::Buy;
    if (!level) {
        level = asks_.find_level(order->price);
        side = Side::Sell;
    }

    assert(level != nullptr);

    // Remove from intrusive list and update stats
    level->orders.remove(order);
    level->total_qty -= order->remaining_qty;
    level->order_count--;

    // Notify listener before deallocation
    if (listener_) {
        listener_->on_order_cancelled(*order);
    }

    // Clean up empty price level
    if (level->empty()) {
        if (side == Side::Buy) {
            bids_.remove_level(level, level_pool_);
        } else {
            asks_.remove_level(level, level_pool_);
        }
        if (listener_) {
            listener_->on_price_level_removed(order->price, side);
        }
    }

    lookup_.erase(order_id);
    order_pool_.deallocate(order);

    return true;
}

bool LimitOrderBook::modify_qty(const OrderId& order_id, const Quantity& new_qty, uint64_t priority_seq) noexcept {
    LiveOrder* order = lookup_.find(order_id);
    if (!order) return false;

    PriceLevelNode* level = bids_.find_level(order->price);
    if (!level) {
        level = asks_.find_level(order->price);
    }

    assert(level != nullptr);

    if (new_qty < order->remaining_qty) {
        // Decrease quantity: retains priority
        level->total_qty -= (order->remaining_qty - new_qty);
        order->remaining_qty = new_qty;
    } else if (new_qty > order->remaining_qty) {
        // Increase quantity: loses priority, moves to tail
        level->total_qty += (new_qty - order->remaining_qty);
        order->remaining_qty = new_qty;
        order->priority_seq = priority_seq;

        level->orders.remove(order);
        level->orders.append(order);
    }

    return true;
}

void LimitOrderBook::fill_order(LiveOrder* order, const Quantity& filled_qty) noexcept {
    assert(order != nullptr);
    assert(filled_qty <= order->remaining_qty);

    PriceLevelNode* level = bids_.find_level(order->price);
    Side side = Side::Buy;
    if (!level) {
        level = asks_.find_level(order->price);
        side = Side::Sell;
    }

    assert(level != nullptr);

    level->total_qty -= filled_qty;
    order->remaining_qty -= filled_qty;

    if (listener_) {
        listener_->on_order_filled(*order, filled_qty);
    }

    if (order->remaining_qty.units() == 0) {
        // Order fully filled: cancel/remove it
        level->orders.remove(order);
        level->order_count--;

        if (level->empty()) {
            if (side == Side::Buy) {
                bids_.remove_level(level, level_pool_);
            } else {
                asks_.remove_level(level, level_pool_);
            }
            if (listener_) {
                listener_->on_price_level_removed(order->price, side);
            }
        }

        lookup_.erase(order->key.order_id);
        order_pool_.deallocate(order);
    }
}

Price LimitOrderBook::best_bid() const noexcept {
    return bids_.empty() ? Price(0) : bids_.best_level()->price;
}

Price LimitOrderBook::best_ask() const noexcept {
    return asks_.empty() ? Price(0) : asks_.best_level()->price;
}

void LimitOrderBook::clear() noexcept {
    bids_.clear(level_pool_, order_pool_);
    asks_.clear(level_pool_, order_pool_);
    lookup_.clear();
}

void LimitOrderBook::verify_invariants() const noexcept {
    bids_.verify_invariants();
    asks_.verify_invariants();
}

} // namespace fluxtrade
