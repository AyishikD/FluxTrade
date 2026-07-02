#include <matching/matching_engine.hpp>
#include <kernel/clock.hpp>
#include <algorithm>

namespace fluxtrade {

MatchingEngine::MatchingEngine(IOrderBookListener* listener) noexcept
    : book_(listener) {}

MatchingResult MatchingEngine::submit_order(const Order& order, ExecutionContext& ctx) noexcept {
    const uint64_t start_time = Clock::now_steady();
    stats_.orders_received++;

    MatchingResult res;
    res.status = MatchingStatus::Success;

    const uint64_t timestamp = Clock::now_steady();

    // 1. Generate ACK (Order Accepted)
    TradeBuilder::build_ack(ctx, exec_seq_++, order, timestamp);

    Quantity remaining_qty = order.qty;

    // 2. Perform price-time priority crossing matches
    if (order.side == Side::Buy) {
        while (remaining_qty.units() > 0 && book_.best_ask_level()) {
            PriceLevelNode* level = book_.best_ask_level();
            
            // Check cross boundary (Aggressive Buy Price >= Best resting Ask Price)
            if (order.type == OrderType::Limit && order.price < level->price) {
                break;
            }

            // Match against resting orders at this level
            LiveOrder* resting = level->orders.head();
            while (resting && remaining_qty.units() > 0) {
                LiveOrder* next_resting = resting->next;

                if (!can_self_trade(order, *resting)) {
                    resting = next_resting;
                    continue;
                }

                const uint64_t match_qty = std::min(remaining_qty.units(), resting->remaining_qty.units());
                const Quantity match_qty_obj(match_qty);
                const Price match_price = level->price;

                const Quantity remaining_maker(resting->remaining_qty.units() - match_qty);
                const Quantity remaining_taker(remaining_qty.units() - match_qty);

                const uint64_t taker_seq = exec_seq_++;
                const uint64_t maker_seq = exec_seq_++;

                // Build Trade and Taker/Maker Execution Reports (before fill modifies/deallocates resting node)
                TradeBuilder::build_match(
                    ctx,
                    trade_seq_++,
                    taker_seq,
                    maker_seq,
                    order,
                    *resting,
                    match_qty_obj,
                    match_price,
                    remaining_taker,
                    remaining_maker,
                    timestamp
                );

                res.trades_generated++;
                stats_.trades_executed++;
                stats_.shares_executed += match_qty;
                stats_.total_traded_volume += match_qty * match_price.ticks();

                // Apply fill to resting order book
                book_.fill_order(resting, match_qty_obj);

                remaining_qty = remaining_taker;
                resting = next_resting;
            }

            // If the level has no orders, book unlinking is handled by fill_order
        }
    } else {
        // Sell Side Matching
        while (remaining_qty.units() > 0 && book_.best_bid_level()) {
            PriceLevelNode* level = book_.best_bid_level();

            // Check cross boundary (Aggressive Sell Price <= Best resting Bid Price)
            if (order.type == OrderType::Limit && order.price > level->price) {
                break;
            }

            LiveOrder* resting = level->orders.head();
            while (resting && remaining_qty.units() > 0) {
                LiveOrder* next_resting = resting->next;

                if (!can_self_trade(order, *resting)) {
                    resting = next_resting;
                    continue;
                }

                const uint64_t match_qty = std::min(remaining_qty.units(), resting->remaining_qty.units());
                const Quantity match_qty_obj(match_qty);
                const Price match_price = level->price;

                const Quantity remaining_maker(resting->remaining_qty.units() - match_qty);
                const Quantity remaining_taker(remaining_qty.units() - match_qty);

                const uint64_t taker_seq = exec_seq_++;
                const uint64_t maker_seq = exec_seq_++;

                TradeBuilder::build_match(
                    ctx,
                    trade_seq_++,
                    taker_seq,
                    maker_seq,
                    order,
                    *resting,
                    match_qty_obj,
                    match_price,
                    remaining_taker,
                    remaining_maker,
                    timestamp
                );

                res.trades_generated++;
                stats_.trades_executed++;
                stats_.shares_executed += match_qty;
                stats_.total_traded_volume += match_qty * match_price.ticks();

                book_.fill_order(resting, match_qty_obj);

                remaining_qty = remaining_taker;
                resting = next_resting;
            }
        }
    }

    // 3. Post-match actions for remaining aggressive quantity
    if (remaining_qty.units() > 0) {
        if (order.type == OrderType::Limit) {
            // Insert limit order remainder as resting
            book_.insert(order, order.seq_num);
            res.status = (remaining_qty == order.qty) ? MatchingStatus::Success : MatchingStatus::PartiallyFilled;
        } else if (order.type == OrderType::Market) {
            // Cancel remaining market order balance
            TradeBuilder::build_expiration(ctx, exec_seq_++, order, remaining_qty, timestamp);
            res.status = MatchingStatus::Expired;
        }
    } else {
        res.status = MatchingStatus::Filled;
        stats_.orders_matched++;
    }

    // 4. Record matching latency
    const uint64_t elapsed = Clock::now_steady() - start_time;
    stats_.record_latency(elapsed);

    return res;
}

bool MatchingEngine::cancel_order(
    const OrderId& order_id,
    ClientId client_id,
    AccountId account_id,
    SymbolId symbol_id,
    ExecutionContext& ctx
) noexcept {
    const uint64_t start_time = Clock::now_steady();
    const uint64_t timestamp = Clock::now_steady();

    LiveOrder* order = book_.find_order(order_id);
    if (!order) {
        stats_.orders_rejected++;
        return false;
    }

    // Generate cancellation report before removing order
    TradeBuilder::build_cancel(ctx, exec_seq_++, *order, client_id, account_id, symbol_id, timestamp);

    book_.cancel(order_id);
    stats_.orders_cancelled++;

    const uint64_t elapsed = Clock::now_steady() - start_time;
    stats_.record_latency(elapsed);

    return true;
}

bool MatchingEngine::modify_order(const OrderId& order_id, const Quantity& new_qty, ExecutionContext& ctx) noexcept {
    const uint64_t start_time = Clock::now_steady();

    // In this basic version, we delegate modify priority shifts to the LOB, 
    // maintaining current sequence metrics inside the engine
    bool res = book_.modify_qty(order_id, new_qty, Clock::now_steady());

    const uint64_t elapsed = Clock::now_steady() - start_time;
    stats_.record_latency(elapsed);

    return res;
}

void MatchingEngine::reset() noexcept {
    trade_seq_ = 1;
    exec_seq_ = 1;
    book_.clear();
    stats_ = MatchingStatistics{};
}

bool MatchingEngine::can_self_trade(const Order& aggressive, const LiveOrder& resting) const noexcept {
    // Basic STP placeholder (always allow trade crossing)
    return true;
}

} // namespace fluxtrade
