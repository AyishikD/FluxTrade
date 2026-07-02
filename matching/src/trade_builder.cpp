#include <matching/trade_builder.hpp>

namespace fluxtrade {

void TradeBuilder::build_match(
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
) noexcept {
    // 1. Populate Trade
    Trade* t = ctx.add_trade();
    if (t) {
        t->trade_id = TradeId(trade_seq);
        t->buy_order_id = (aggressive.side == Side::Buy) ? aggressive.order_id : resting.key.order_id;
        t->sell_order_id = (aggressive.side == Side::Buy) ? resting.key.order_id : aggressive.order_id;
        t->price = match_price;
        t->qty = match_qty;
        t->timestamp = timestamp;
        t->symbol_id = aggressive.symbol_id;
    }

    // 2. Populate Taker Execution Report
    ExecutionReport* r_taker = ctx.add_report();
    if (r_taker) {
        r_taker->exec_id = ExecutionId(exec_seq_taker);
        r_taker->order_id = aggressive.order_id;
        r_taker->client_id = aggressive.client_id;
        r_taker->account_id = aggressive.account_id;
        r_taker->symbol_id = aggressive.symbol_id;
        r_taker->price = match_price;
        r_taker->last_qty = match_qty;
        r_taker->remaining_qty = remaining_qty_taker;
        r_taker->timestamp = timestamp;
        r_taker->exec_type = ExecutionType::Trade;
        r_taker->reject_reason = RejectReason::None;
    }

    // 3. Populate Maker Execution Report
    ExecutionReport* r_maker = ctx.add_report();
    if (r_maker) {
        r_maker->exec_id = ExecutionId(exec_seq_maker);
        r_maker->order_id = resting.key.order_id;
        // Maker client info is preserved via the resting structure or standard routing context.
        // We assume resting ClientId mapping or similar, if not provided we keep ClientId(0) or resting keys.
        r_maker->client_id = ClientId(0); 
        r_maker->account_id = AccountId(0);
        r_maker->symbol_id = resting.key.symbol_id;
        r_maker->price = match_price;
        r_maker->last_qty = match_qty;
        r_maker->remaining_qty = remaining_qty_maker;
        r_maker->timestamp = timestamp;
        r_maker->exec_type = ExecutionType::Trade;
        r_maker->reject_reason = RejectReason::None;
    }
}

void TradeBuilder::build_ack(
    ExecutionContext& ctx,
    uint64_t exec_seq,
    const Order& order,
    uint64_t timestamp
) noexcept {
    ExecutionReport* r = ctx.add_report();
    if (r) {
        r->exec_id = ExecutionId(exec_seq);
        r->order_id = order.order_id;
        r->client_id = order.client_id;
        r->account_id = order.account_id;
        r->symbol_id = order.symbol_id;
        r->price = order.price;
        r->last_qty = Quantity(0);
        r->remaining_qty = order.qty;
        r->timestamp = timestamp;
        r->exec_type = ExecutionType::New;
        r->reject_reason = RejectReason::None;
    }
}

void TradeBuilder::build_cancel(
    ExecutionContext& ctx,
    uint64_t exec_seq,
    const LiveOrder& order,
    ClientId client_id,
    AccountId account_id,
    SymbolId symbol_id,
    uint64_t timestamp
) noexcept {
    ExecutionReport* r = ctx.add_report();
    if (r) {
        r->exec_id = ExecutionId(exec_seq);
        r->order_id = order.key.order_id;
        r->client_id = client_id;
        r->account_id = account_id;
        r->symbol_id = symbol_id;
        r->price = order.price;
        r->last_qty = Quantity(0);
        r->remaining_qty = Quantity(0);
        r->timestamp = timestamp;
        r->exec_type = ExecutionType::Cancel;
        r->reject_reason = RejectReason::None;
    }
}

void TradeBuilder::build_reject(
    ExecutionContext& ctx,
    uint64_t exec_seq,
    const Order& order,
    RejectReason reason,
    uint64_t timestamp
) noexcept {
    ExecutionReport* r = ctx.add_report();
    if (r) {
        r->exec_id = ExecutionId(exec_seq);
        r->order_id = order.order_id;
        r->client_id = order.client_id;
        r->account_id = order.account_id;
        r->symbol_id = order.symbol_id;
        r->price = order.price;
        r->last_qty = Quantity(0);
        r->remaining_qty = Quantity(0);
        r->timestamp = timestamp;
        r->exec_type = ExecutionType::Reject;
        r->reject_reason = reason;
    }
}

void TradeBuilder::build_expiration(
    ExecutionContext& ctx,
    uint64_t exec_seq,
    const Order& order,
    const Quantity& expired_qty,
    uint64_t timestamp
) noexcept {
    ExecutionReport* r = ctx.add_report();
    if (r) {
        r->exec_id = ExecutionId(exec_seq);
        r->order_id = order.order_id;
        r->client_id = order.client_id;
        r->account_id = order.account_id;
        r->symbol_id = order.symbol_id;
        r->price = order.price;
        r->last_qty = Quantity(0);
        r->remaining_qty = Quantity(0);
        r->timestamp = timestamp;
        r->exec_type = ExecutionType::Cancel; // Overloaded as Cancel type
        r->reject_reason = RejectReason::None;
    }
}

} // namespace fluxtrade
