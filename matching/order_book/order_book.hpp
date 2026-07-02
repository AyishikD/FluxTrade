#pragma once

#include <domain/order.hpp>
#include <matching/order_book/bid_book.hpp>
#include <matching/order_book/ask_book.hpp>
#include <matching/order_book/order_lookup.hpp>

namespace fluxtrade {
namespace persistence {
    class SnapshotWriter;
    class SnapshotReader;
}

// Interface to capture book mutations for logs, replay, and market data publishers
class IOrderBookListener {
public:
    virtual ~IOrderBookListener() = default;
    virtual void on_order_inserted(const LiveOrder& order) noexcept = 0;
    virtual void on_order_cancelled(const LiveOrder& order) noexcept = 0;
    virtual void on_order_filled(const LiveOrder& order, const Quantity& filled_qty) noexcept = 0;
    virtual void on_price_level_created(const Price& price, Side side) noexcept = 0;
    virtual void on_price_level_removed(const Price& price, Side side) noexcept = 0;
};

class LimitOrderBook {
public:
    friend class persistence::SnapshotWriter;
    friend class persistence::SnapshotReader;

    explicit LimitOrderBook(IOrderBookListener* listener = nullptr) noexcept;
    ~LimitOrderBook() = default;

    LimitOrderBook(const LimitOrderBook&) = delete;
    LimitOrderBook& operator=(const LimitOrderBook&) = delete;

    // Inserts a new client order into the book. Complexity: O(log L) if price level created, otherwise O(1).
    LiveOrder* insert(const Order& order, uint64_t priority_seq) noexcept;

    // Cancels an active order. Complexity: O(1) via hash lookup + intrusive list unlinking.
    bool cancel(const OrderId& order_id) noexcept;

    // Modifies an order quantity. If quantity increases, order loses priority. Complexity: O(1).
    bool modify_qty(const OrderId& order_id, const Quantity& new_qty, uint64_t priority_seq) noexcept;

    // Records a fill (partial or full) against an active order. Complexity: O(1).
    void fill_order(LiveOrder* order, const Quantity& filled_qty) noexcept;

    [[nodiscard]] Price best_bid() const noexcept;
    [[nodiscard]] Price best_ask() const noexcept;

    [[nodiscard]] PriceLevelNode* best_bid_level() const noexcept { return bids_.best_level(); }
    [[nodiscard]] PriceLevelNode* best_ask_level() const noexcept { return asks_.best_level(); }

    [[nodiscard]] LiveOrder* find_order(const OrderId& id) const noexcept { return lookup_.find(id); }

    void verify_invariants() const noexcept;
    void clear() noexcept;

private:
    IOrderBookListener* listener_ = nullptr;

    BidBook bids_;
    AskBook asks_;
    OrderLookup lookup_;

    // Block pools for zero-heap runtime executions
    ObjectPool<PriceLevelNode, 1024> level_pool_;
    ObjectPool<LiveOrder, 10240> order_pool_;
};

} // namespace fluxtrade
