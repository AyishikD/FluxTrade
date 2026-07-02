#pragma once

#include <risk/risk_context.hpp>
#include <risk/risk_result.hpp>
#include <risk/risk_limits.hpp>
#include <risk/risk_statistics.hpp>
#include <risk/credit_manager.hpp>
#include <risk/margin_manager.hpp>
#include <risk/duplicate_validator.hpp>
#include <risk/rate_limit_validator.hpp>
#include <risk/kill_switch.hpp>
#include <risk/circuit_breaker.hpp>
#include <risk/fat_finger_validator.hpp>
#include <risk/price_band_validator.hpp>
#include <risk/self_trade_prevention.hpp>
#include <common/expected.hpp>
#include <kernel/clock.hpp>

namespace fluxtrade {

// Compile-Time Policy composed Risk Pipeline
template <typename... Policies>
class RiskPipeline {
public:
    template <typename EngineContext>
    [[nodiscard]] static RiskResult validate(
        const RiskContext& ctx,
        const RiskLimits& limits,
        EngineContext& eng_ctx,
        RiskTrace& trace
    ) noexcept {
        RiskResult res;
        res.decision = RiskDecision::Accept;
        res.reason = RejectReason::None;
        
        const uint64_t start = Clock::now_steady();
        
        // Evaluate each policy sequentially in compilation time
        bool ok = (validate_policy<Policies>(ctx, limits, eng_ctx, res, trace) && ...);
        
        res.decision = ok ? RiskDecision::Accept : RiskDecision::Reject;
        res.timestamp = Clock::now_steady();
        res.pipeline_latency_ns = res.timestamp - start;
        return res;
    }

private:
    template <typename Policy, typename EngineContext>
    [[nodiscard]] static bool validate_policy(
        const RiskContext& ctx,
        const RiskLimits& limits,
        EngineContext& eng_ctx,
        RiskResult& res,
        RiskTrace& trace
    ) noexcept {
#ifdef FLUX_RISK_TRACE_FINE
        const uint64_t start = Clock::now_steady();
#endif
        const bool ok = Policy::validate(ctx, limits, eng_ctx);
        
#ifdef FLUX_RISK_TRACE_FINE
        const uint64_t latency = Clock::now_steady() - start;
        const RiskDecision dec = ok ? RiskDecision::Accept : RiskDecision::Reject;
        trace.add_stage(Policy::name(), static_cast<uint32_t>(latency), dec);
#else
        const RiskDecision dec = ok ? RiskDecision::Accept : RiskDecision::Reject;
        trace.add_stage(Policy::name(), 0, dec);
#endif

        if (!ok) {
            res.reason = Policy::reject_reason();
            return false;
        }
        return true;
    }
};

// Concrete policy adapters
struct KillSwitchPolicy {
    static constexpr const char* name() { return "KillSwitch"; }
    static constexpr RejectReason reject_reason() { return RejectReason::RiskLimitExceeded; }
    template <typename EngineContext>
    static bool validate(const RiskContext& ctx, const RiskLimits& limits, EngineContext&) noexcept {
        return KillSwitch::validate(ctx, limits, AccountState{}, PositionState{});
    }
};

struct CircuitBreakerPolicy {
    static constexpr const char* name() { return "CircuitBreaker"; }
    static constexpr RejectReason reject_reason() { return RejectReason::RiskLimitExceeded; }
    template <typename EngineContext>
    static bool validate(const RiskContext& ctx, const RiskLimits& limits, EngineContext&) noexcept {
        return CircuitBreaker::validate(ctx, limits, AccountState{}, PositionState{});
    }
};

struct DuplicatePolicy {
    static constexpr const char* name() { return "DuplicateCheck"; }
    static constexpr RejectReason reject_reason() { return RejectReason::DuplicateOrder; }
    template <typename EngineContext>
    static bool validate(const RiskContext& ctx, const RiskLimits&, EngineContext& eng_ctx) noexcept {
        return eng_ctx.duplicates.check_duplicate(ctx.client_order_id);
    }
};

struct RateLimitPolicy {
    static constexpr const char* name() { return "RateLimit"; }
    static constexpr RejectReason reject_reason() { return RejectReason::RiskLimitExceeded; }
    template <typename EngineContext>
    static bool validate(const RiskContext& ctx, const RiskLimits& limits, EngineContext& eng_ctx) noexcept {
        return eng_ctx.rate_limits.check_rate(ctx.account_id.get(), ctx.timestamp, limits.max_orders_per_sec);
    }
};

struct FatFingerPolicy {
    static constexpr const char* name() { return "FatFinger"; }
    static constexpr RejectReason reject_reason() { return RejectReason::InvalidQuantity; }
    template <typename EngineContext>
    static bool validate(const RiskContext& ctx, const RiskLimits& limits, EngineContext&) noexcept {
        return FatFingerValidator::validate(ctx, limits, AccountState{}, PositionState{});
    }
};

struct PriceBandPolicy {
    static constexpr const char* name() { return "PriceBand"; }
    static constexpr RejectReason reject_reason() { return RejectReason::InvalidPrice; }
    template <typename EngineContext>
    static bool validate(const RiskContext& ctx, const RiskLimits& limits, EngineContext&) noexcept {
        return PriceBandValidator::validate(ctx, limits, AccountState{}, PositionState{});
    }
};

struct STPPolicy {
    static constexpr const char* name() { return "SelfTrade"; }
    static constexpr RejectReason reject_reason() { return RejectReason::RiskLimitExceeded; }
    template <typename EngineContext>
    static bool validate(const RiskContext& ctx, const RiskLimits& limits, EngineContext&) noexcept {
        return STPValidator::validate(ctx, limits, AccountState{}, PositionState{});
    }
};

struct PositionPolicy {
    static constexpr const char* name() { return "PositionLimit"; }
    static constexpr RejectReason reject_reason() { return RejectReason::RiskLimitExceeded; }
    template <typename EngineContext>
    static bool validate(const RiskContext& ctx, const RiskLimits& limits, EngineContext& eng_ctx) noexcept {
        PositionState* pos = eng_ctx.margin.get_position_state(ctx.account_id.get(), ctx.symbol_id.get());
        return MarginManager::validate(ctx, limits, AccountState{}, *pos);
    }
};

struct CreditPolicy {
    static constexpr const char* name() { return "CreditLimit"; }
    static constexpr RejectReason reject_reason() { return RejectReason::RiskLimitExceeded; }
    template <typename EngineContext>
    static bool validate(const RiskContext& ctx, const RiskLimits& limits, EngineContext& eng_ctx) noexcept {
        AccountState* acc = eng_ctx.credit.get_account_state(ctx.account_id.get());
        return CreditManager::validate(ctx, limits, *acc, PositionState{});
    }
};

// Full compiled pre-trade validation pipeline composition
using PreTradeRiskPipeline = RiskPipeline<
    KillSwitchPolicy,
    CircuitBreakerPolicy,
    DuplicatePolicy,
    RateLimitPolicy,
    FatFingerPolicy,
    PriceBandPolicy,
    STPPolicy,
    PositionPolicy,
    CreditPolicy
>;

class RiskEngine {
public:
    RiskEngine() noexcept;
    ~RiskEngine() = default;

    RiskEngine(const RiskEngine&) = delete;
    RiskEngine& operator=(const RiskEngine&) = delete;

    // Evaluates an inbound context through the policy pipeline. O(1) validations.
    [[nodiscard]] expected<RiskResult, RejectReason> validate(const RiskContext& ctx, RiskTrace& trace) noexcept;

    void update_on_fill(uint32_t account_id, uint32_t symbol_id, Side side, uint64_t qty, uint64_t price_ticks) noexcept;
    void update_on_cancel(uint32_t account_id, uint64_t cost) noexcept;

    [[nodiscard]] const RiskLimits& limits() const noexcept { return limits_; }
    [[nodiscard]] RiskLimits& limits() noexcept { return limits_; }

    [[nodiscard]] const RiskStatistics& stats() const noexcept { return stats_; }
    [[nodiscard]] RiskStatistics& stats() noexcept { return stats_; }

    // Struct context exposed to pipeline policies
    struct PipelineContext {
        CreditManager credit;
        MarginManager margin;
        DuplicateValidator duplicates;
        RateLimitValidator rate_limits;
    } context;

    void reset() noexcept;

private:
    RiskLimits limits_;
    RiskStatistics stats_;
};

} // namespace fluxtrade
