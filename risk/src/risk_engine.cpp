#include <risk/risk_engine.hpp>

namespace fluxtrade {

RiskEngine::RiskEngine() noexcept {
    reset();
}

expected<RiskResult, RejectReason> RiskEngine::validate(const RiskContext& ctx, RiskTrace& trace) noexcept {
    const uint64_t start_time = Clock::now_steady();

    // 1. Run compiled pipeline
    RiskResult res = PreTradeRiskPipeline::validate(ctx, limits_, context, trace);

    // 2. Process results
    if (res.decision == RiskDecision::Accept) {
        stats_.accepted_count++;
        // Reserve buying power (scaled to price ticks)
        const uint64_t cost = (ctx.price.ticks() * ctx.qty.units()) / 1000000ULL;
        context.credit.update_on_accept(ctx.account_id.get(), cost);

        const uint64_t elapsed = Clock::now_steady() - start_time;
        stats_.record_latency(elapsed);
        return res;
    } else {
        stats_.rejected_count++;
        
        // Map reject reason and increment the specific validator counter
        if (res.reason == RejectReason::DuplicateOrder) {
            stats_.duplicate_rejects++;
        } else if (res.reason == RejectReason::InvalidPrice) {
            stats_.price_band_rejects++;
        } else if (res.reason == RejectReason::InvalidQuantity) {
            stats_.fat_finger_rejects++;
        } else if (res.reason == RejectReason::RiskLimitExceeded) {
            // Find which stage failed in the trace
            for (size_t i = 0; i < trace.stage_count; ++i) {
                if (trace.stages[i].decision == RiskDecision::Reject) {
                    const std::string name(trace.stages[i].stage_name);
                    if (name == "KillSwitch") {
                        stats_.kill_switch_rejects++;
                    } else if (name == "CircuitBreaker") {
                        stats_.circuit_breaker_rejects++;
                    } else if (name == "RateLimit") {
                        stats_.rate_limit_rejects++;
                    } else if (name == "SelfTrade") {
                        stats_.stp_rejects++;
                    } else if (name == "PositionLimit") {
                        stats_.position_rejects++;
                    } else if (name == "CreditLimit") {
                        stats_.credit_rejects++;
                    }
                    break;
                }
            }
        }

        const uint64_t elapsed = Clock::now_steady() - start_time;
        stats_.record_latency(elapsed);
        return unexpected<RejectReason>(res.reason);
    }
}

void RiskEngine::update_on_fill(
    uint32_t account_id,
    uint32_t symbol_id,
    Side side,
    uint64_t qty,
    uint64_t price_ticks
) noexcept {
    context.margin.update_on_fill(account_id, symbol_id, side, qty);
    context.credit.update_on_fill(account_id, qty * price_ticks);
}

void RiskEngine::update_on_cancel(uint32_t account_id, uint64_t cost) noexcept {
    context.credit.update_on_cancel(account_id, cost);
}

void RiskEngine::reset() noexcept {
    stats_ = RiskStatistics{};
    
    // Default dynamic limits configurations (scaled to match fixed-point value objects)
    limits_.max_credit_limit = 1000000000000000ULL; // $10 million in ticks
    limits_.max_daily_exposure = 2000000000000000ULL; // $20 million daily exposure ticks
    limits_.max_order_qty = 10000000000ULL; // 10,000 max quantity units
    limits_.max_order_price = 500000000000ULL; // $5,000 max ticks price
    limits_.price_band_tolerance = 0.10;
    limits_.stp_mode = SelfTradePreventionMode::CancelNew;
    limits_.max_orders_per_sec = 100;
    limits_.kill_switch_active = false;

    // Reset status locks
    KillSwitch::set_global_halt(false);
    for (uint32_t i = 0; i < 1024; ++i) {
        KillSwitch::set_account_halt(i, false);
        KillSwitch::set_symbol_halt(i, false);
    }
    CircuitBreaker::set_status(MarketStatus::Trading);

    context.credit.reset();
    context.margin.reset();
    context.duplicates.reset();
    context.rate_limits.reset();
}

} // namespace fluxtrade
