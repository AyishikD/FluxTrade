#pragma once

#include <matching/execution_context.hpp>
#include <domain/order.hpp>
#include <domain/order_view.hpp>

namespace fluxtrade {

class TradeBuilder {
public:
    TradeBuilder() = default;
    ~TradeBuilder() = default;

    TradeBuilder(const TradeBuilder&) = delete;
    TradeBuilder& operator=(const TradeBuilder&) = delete;

    // Populates a Trade record and two ExecutionReports (one for maker, one for taker)
    static void build_match(
        ExecutionContext& ctx,
        uint64_t trade_seq,
        uint64_t exec_seq_taker,
        uint64_t exec_seq_maker,
        const Order& aggressive,
        const LiveOrder& resting,
        const Quantity& match_qty,
        const Price& match_price,
        const Quantity& remaining_qty_taker,
        const Quantity& remaining_qty_maker,
        uint64_t timestamp
    ) noexcept;

    // Populates an ExecutionReport for an order acknowledgement (New)
    static void build_ack(
        ExecutionContext& ctx,
        uint64_t exec_seq,
        const Order& order,
        uint64_t timestamp
    ) noexcept;

    // Populates an ExecutionReport for an order cancellation
    static void build_cancel(
        ExecutionContext& ctx,
        uint64_t exec_seq,
        const LiveOrder& order,
        ClientId client_id,
        AccountId account_id,
        SymbolId symbol_id,
        uint64_t timestamp
    ) noexcept;

    // Populates an ExecutionReport for an order rejection
    static void build_reject(
        ExecutionContext& ctx,
        uint64_t exec_seq,
        const Order& order,
        RejectReason reason,
        uint64_t timestamp
    ) noexcept;

    // Populates an ExecutionReport for an order expiration (e.g. Market order balance)
    static void build_expiration(
        ExecutionContext& ctx,
        uint64_t exec_seq,
        const Order& order,
        const Quantity& expired_qty,
        uint64_t timestamp
    ) noexcept;
};

} // namespace fluxtrade
