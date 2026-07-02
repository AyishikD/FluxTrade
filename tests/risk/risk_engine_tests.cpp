#include <gtest/gtest.h>
#include <risk/risk_engine.hpp>

using namespace fluxtrade;

// Helper to construct a standard RiskContext frame
RiskContext make_risk_context(
    uint64_t id,
    double price,
    double qty,
    uint32_t account_id,
    uint32_t symbol_id,
    Side side = Side::Buy,
    double ref_price = 0.0,
    uint64_t client_order_id = 0
) {
    RiskContext ctx;
    ctx.order_id = OrderId(id);
    ctx.price = Price::from_double(price);
    ctx.qty = Quantity::from_double(qty);
    ctx.timestamp = Clock::now_steady();
    ctx.reference_price = Price::from_double(ref_price);
    ctx.client_order_id = client_order_id;
    ctx.symbol_id = SymbolId(symbol_id);
    ctx.client_id = ClientId(1);
    ctx.account_id = AccountId(account_id);
    ctx.side = side;
    ctx.type = OrderType::Limit;
    return ctx;
}

TEST(RiskEngineTest, NormalAcceptPath) {
    RiskEngine engine;
    RiskTrace trace;

    RiskContext ctx = make_risk_context(1, 100.50, 10.0, 1, 1);
    auto res = engine.validate(ctx, trace);

    if (!res.has_value()) {
        printf("--- RISK TRACE STAGES ---\n");
        for (size_t i = 0; i < trace.stage_count; ++i) {
            printf("Stage: %s | Decision: %d | Latency: %u ns\n",
                   trace.stages[i].stage_name,
                   static_cast<int>(trace.stages[i].decision),
                   trace.stages[i].latency_ns);
        }
        printf("-------------------------\n");
    }

    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res.value().decision, RiskDecision::Accept);

    // Verify stats
    EXPECT_EQ(engine.stats().accepted_count, 1);
    EXPECT_EQ(engine.stats().rejected_count, 0);

    // Reserved capital should update (100.50 * 10 = 1005 ticks = 1,005,000,000 scaled ticks or cost units)
    AccountState* acc = engine.context.credit.get_account_state(1);
    EXPECT_GT(acc->reserved_capital, 0);
}

TEST(RiskEngineTest, CreditLimitExceeded) {
    RiskEngine engine;
    RiskTrace trace;

    // Set available buying power to a very small amount
    AccountState* acc = engine.context.credit.get_account_state(1);
    acc->available_buying_power = 100; // very low

    RiskContext ctx = make_risk_context(1, 100.50, 10.0, 1, 1);
    auto res = engine.validate(ctx, trace);

    if (!res.has_value()) {
        printf("--- CreditLimitExceeded TRACE ---\n");
        for (size_t i = 0; i < trace.stage_count; ++i) {
            printf("Stage: %s | Decision: %d\n", trace.stages[i].stage_name, static_cast<int>(trace.stages[i].decision));
        }
    }

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), RejectReason::RiskLimitExceeded);
    EXPECT_EQ(engine.stats().rejected_count, 1);
    EXPECT_EQ(engine.stats().credit_rejects, 1);
}

TEST(RiskEngineTest, PositionLimitExceeded) {
    RiskEngine engine;
    RiskTrace trace;

    // Set max long limit on Symbol 1 to 50 shares
    PositionState* pos = engine.context.margin.get_position_state(1, 1);
    pos->max_long_limit = 50;

    // Submit order for 60 shares
    RiskContext ctx = make_risk_context(1, 100.50, 60.0, 1, 1, Side::Buy);
    auto res = engine.validate(ctx, trace);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), RejectReason::RiskLimitExceeded);
    EXPECT_EQ(engine.stats().position_rejects, 1);
}

TEST(RiskEngineTest, FatFingerCeilingRejection) {
    RiskEngine engine;
    RiskTrace trace;

    // Configure Fat Finger ceiling to max 1,000 units
    engine.limits().max_order_qty = 1000;

    RiskContext ctx = make_risk_context(1, 100.50, 2000.0, 1, 1); // 2,000 units > 1,000 limit
    auto res = engine.validate(ctx, trace);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), RejectReason::InvalidQuantity);
    EXPECT_EQ(engine.stats().fat_finger_rejects, 1);
}

TEST(RiskEngineTest, PriceBandViolation) {
    RiskEngine engine;
    RiskTrace trace;

    // Price band deviation = 10%
    engine.limits().price_band_tolerance = 0.10;

    // Reference Price = 100.00. Dynamic limits: [90.00, 110.00]
    // Order Price = 115.00 (violation!)
    RiskContext ctx = make_risk_context(1, 115.00, 10.0, 1, 1, Side::Buy, 100.00);
    auto res = engine.validate(ctx, trace);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), RejectReason::InvalidPrice);
    EXPECT_EQ(engine.stats().price_band_rejects, 1);
}

TEST(RiskEngineTest, GlobalKillSwitchHalt) {
    RiskEngine engine;
    RiskTrace trace;

    // Activate global halt
    KillSwitch::set_global_halt(true);

    RiskContext ctx = make_risk_context(1, 100.50, 10.0, 1, 1);
    auto res = engine.validate(ctx, trace);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), RejectReason::RiskLimitExceeded);
    EXPECT_EQ(engine.stats().kill_switch_rejects, 1);

    // Turn off global halt
    KillSwitch::set_global_halt(false);
    ctx.order_id = OrderId(2);
    auto res2 = engine.validate(ctx, trace);
    EXPECT_TRUE(res2.has_value());
}

TEST(RiskEngineTest, AccountKillSwitchHalt) {
    RiskEngine engine;

    // Halt Account 2
    KillSwitch::set_account_halt(2, true);

    RiskContext ctx1 = make_risk_context(1, 100.50, 10.0, 1, 1); // Account 1
    RiskContext ctx2 = make_risk_context(2, 100.50, 10.0, 2, 1); // Account 2

    RiskTrace trace1;
    RiskTrace trace2;
    auto res1 = engine.validate(ctx1, trace1);
    auto res2 = engine.validate(ctx2, trace2);

    if (!res2.has_value()) {
        printf("--- AccountKillSwitchHalt TRACE ---\n");
        for (size_t i = 0; i < trace2.stage_count; ++i) {
            printf("Stage: %s | Decision: %d\n", trace2.stages[i].stage_name, static_cast<int>(trace2.stages[i].decision));
        }
    }

    EXPECT_TRUE(res1.has_value());
    EXPECT_FALSE(res2.has_value());
    EXPECT_EQ(engine.stats().kill_switch_rejects, 1);
}

TEST(RiskEngineTest, CircuitBreakerMarketHalt) {
    RiskEngine engine;
    RiskTrace trace;

    // Set circuit status to Halt
    CircuitBreaker::set_status(MarketStatus::Halt);

    RiskContext ctx = make_risk_context(1, 100.50, 10.0, 1, 1);
    auto res = engine.validate(ctx, trace);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), RejectReason::RiskLimitExceeded);
    EXPECT_EQ(engine.stats().circuit_breaker_rejects, 1);
}

TEST(RiskEngineTest, DuplicateOrderDetection) {
    RiskEngine engine;
    RiskTrace trace;

    // Set ClientOrderId to 9999
    RiskContext ctx1 = make_risk_context(1, 100.50, 10.0, 1, 1, Side::Buy, 0.0, 9999);
    RiskContext ctx2 = make_risk_context(2, 100.50, 10.0, 1, 1, Side::Buy, 0.0, 9999);

    auto res1 = engine.validate(ctx1, trace);
    auto res2 = engine.validate(ctx2, trace);

    EXPECT_TRUE(res1.has_value());
    EXPECT_FALSE(res2.has_value());
    EXPECT_EQ(engine.stats().duplicate_rejects, 1);
}

TEST(RiskEngineTest, OrderRateLimiting) {
    RiskEngine engine;
    RiskTrace trace;

    // Set rate limit to 3 orders/sec
    engine.limits().max_orders_per_sec = 3;

    uint64_t now = Clock::now_steady();
    
    // Send 3 orders (within limit)
    for (int i = 1; i <= 3; ++i) {
        RiskContext ctx = make_risk_context(i, 100.50, 10.0, 1, 1);
        ctx.timestamp = now; // same second
        auto res = engine.validate(ctx, trace);
        EXPECT_TRUE(res.has_value());
    }

    // Send 4th order (violates rate limit)
    RiskContext ctx_violate = make_risk_context(4, 100.50, 10.0, 1, 1);
    ctx_violate.timestamp = now;
    trace.stage_count = 0;
    auto res_violate = engine.validate(ctx_violate, trace);

    if (!res_violate.has_value()) {
        printf("--- OrderRateLimiting TRACE ---\n");
        for (size_t i = 0; i < trace.stage_count; ++i) {
            printf("Stage: %s | Decision: %d\n", trace.stages[i].stage_name, static_cast<int>(trace.stages[i].decision));
        }
    }

    EXPECT_FALSE(res_violate.has_value());
    EXPECT_EQ(engine.stats().rate_limit_rejects, 1);
}

TEST(RiskEngineTest, AuditPipelineTrace) {
    RiskEngine engine;
    RiskTrace trace;

    // Normal acceptance should record all pipeline trace steps
    RiskContext ctx = make_risk_context(1, 100.50, 10.0, 1, 1);
    auto res = engine.validate(ctx, trace);
    (void)res;

    ASSERT_GT(trace.stage_count, 0);
    EXPECT_EQ(std::string(trace.stages[0].stage_name), "KillSwitch");
    EXPECT_EQ(trace.stages[0].decision, RiskDecision::Accept);
}

TEST(RiskEngineTest, DeterministicReplay) {
    RiskEngine engine1;
    RiskEngine engine2;
    RiskTrace trace1;
    RiskTrace trace2;

    std::vector<RiskContext> log = {
        make_risk_context(1, 100.50, 10.0, 1, 1, Side::Buy, 0.0, 1111),
        make_risk_context(2, 100.50, 10.0, 1, 1, Side::Buy, 0.0, 1111), // duplicate
        make_risk_context(3, 100.50, 5000000.0, 1, 1) // fat finger qty limit
    };

    for (const auto& entry : log) {
        auto r1 = engine1.validate(entry, trace1);
        auto r2 = engine2.validate(entry, trace2);
        (void)r1;
        (void)r2;
    }

    // Verify state determinism
    EXPECT_EQ(engine1.stats().accepted_count, engine2.stats().accepted_count);
    EXPECT_EQ(engine1.stats().rejected_count, engine2.stats().rejected_count);
    EXPECT_EQ(engine1.stats().duplicate_rejects, engine2.stats().duplicate_rejects);
    EXPECT_EQ(engine1.stats().fat_finger_rejects, engine2.stats().fat_finger_rejects);
}
