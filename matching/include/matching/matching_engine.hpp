#pragma once

#include <matching/order_book/order_book.hpp>
#include <matching/matching_statistics.hpp>
#include <matching/matching_result.hpp>
#include <matching/trade_builder.hpp>

namespace fluxtrade {

class MatchingEngine {
public:
    explicit MatchingEngine(IOrderBookListener* listener = nullptr) noexcept;
    ~MatchingEngine() = default;

    MatchingEngine(const MatchingEngine&) = delete;
    MatchingEngine& operator=(const MatchingEngine&) = delete;

    // Submits an order for price-time priority execution. Zero dynamic allocation.
    MatchingResult submit_order(const Order& order, ExecutionContext& ctx) noexcept;

    // Cancels an active order in the book.
    bool cancel_order(const OrderId& order_id, ClientId client_id, AccountId account_id, SymbolId symbol_id, ExecutionContext& ctx) noexcept;

    // Modifies an active order's quantity in the book.
    bool modify_order(const OrderId& order_id, const Quantity& new_qty, ExecutionContext& ctx) noexcept;

    [[nodiscard]] const LimitOrderBook& book() const noexcept { return book_; }
    [[nodiscard]] LimitOrderBook& book() noexcept { return book_; }

    [[nodiscard]] const MatchingStatistics& stats() const noexcept { return stats_; }
    [[nodiscard]] MatchingStatistics& stats() noexcept { return stats_; }

    // Resets sequence counters and statistics for replay runs
    void reset() noexcept;

    // Self-Trade Prevention (STP) hook
    [[nodiscard]] bool can_self_trade(const Order& aggressive, const LiveOrder& resting) const noexcept;

private:
    LimitOrderBook book_;
    MatchingStatistics stats_;

    // Monotonically increasing deterministic sequences
    uint64_t trade_seq_ = 1;
    uint64_t exec_seq_ = 1;
};

} // namespace fluxtrade
