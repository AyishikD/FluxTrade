#include <gtest/gtest.h>
#include <matching/matching_engine.hpp>
#include <matching/exchange_simulator.hpp>
#include <vector>

using namespace fluxtrade;

// Helper to construct standard Order frames
Order make_order(
    uint64_t id,
    double price,
    double qty,
    Side side,
    OrderType type = OrderType::Limit,
    TimeInForce tif = TimeInForce::GTC,
    uint64_t seq = 1
) {
    return Order{
        OrderId(id),
        Price::from_double(price),
        Quantity::from_double(qty),
        seq,
        1000,
        1002,
        ClientId(1),
        AccountId(1),
        SymbolId(1),
        side,
        type,
        tif,
        OrderFlags::None
    };
}

TEST(MatchingEngineTest, LimitOrderEntersEmptyBook) {
    MatchingEngine engine;
    ExecutionContext ctx;

    Order o1 = make_order(1, 100.50, 10.0, Side::Buy, OrderType::Limit, TimeInForce::GTC, 1);
    MatchingResult res = engine.submit_order(o1, ctx);

    EXPECT_EQ(res.status, MatchingStatus::Success);
    EXPECT_EQ(res.trades_generated, 0);

    // Book state check
    EXPECT_DOUBLE_EQ(engine.book().best_bid().to_double(), 100.50);
    EXPECT_EQ(ctx.report_count, 1); // ACK
    EXPECT_EQ(ctx.reports[0].exec_type, ExecutionType::New);
}

TEST(MatchingEngineTest, BuyCrossesAskFullFill) {
    MatchingEngine engine;
    ExecutionContext ctx;

    // Resting Ask: 10 shares @ 100.50
    Order o1 = make_order(1, 100.50, 10.0, Side::Sell, OrderType::Limit, TimeInForce::GTC, 1);
    engine.submit_order(o1, ctx);
    ctx.reset();

    // Aggressive Bid: 10 shares @ 100.50
    Order o2 = make_order(2, 100.50, 10.0, Side::Buy, OrderType::Limit, TimeInForce::GTC, 2);
    MatchingResult res = engine.submit_order(o2, ctx);

    EXPECT_EQ(res.status, MatchingStatus::Filled);
    EXPECT_EQ(res.trades_generated, 1);

    // OrderBook should be empty
    EXPECT_DOUBLE_EQ(engine.book().best_bid().to_double(), 0.0);
    EXPECT_DOUBLE_EQ(engine.book().best_ask().to_double(), 0.0);

    // Execution reports: 1 ACK + 1 Taker trade + 1 Maker trade = 3 reports
    ASSERT_EQ(ctx.report_count, 3);
    EXPECT_EQ(ctx.reports[0].exec_type, ExecutionType::New);
    EXPECT_EQ(ctx.reports[1].exec_type, ExecutionType::Trade); // Taker
    EXPECT_EQ(ctx.reports[2].exec_type, ExecutionType::Trade); // Maker
    EXPECT_DOUBLE_EQ(ctx.reports[1].remaining_qty.to_double(), 0.0);
    EXPECT_DOUBLE_EQ(ctx.reports[2].remaining_qty.to_double(), 0.0);
}

TEST(MatchingEngineTest, SellCrossesBidPartialFill) {
    MatchingEngine engine;
    ExecutionContext ctx;

    // Resting Bid: 15 shares @ 100.50
    Order o1 = make_order(1, 100.50, 15.0, Side::Buy, OrderType::Limit, TimeInForce::GTC, 1);
    engine.submit_order(o1, ctx);
    ctx.reset();

    // Aggressive Ask: 10 shares @ 100.50
    Order o2 = make_order(2, 100.50, 10.0, Side::Sell, OrderType::Limit, TimeInForce::GTC, 2);
    MatchingResult res = engine.submit_order(o2, ctx);

    EXPECT_EQ(res.status, MatchingStatus::Filled);
    EXPECT_EQ(res.trades_generated, 1);

    // Remaining resting quantity: 5.0
    ASSERT_NE(engine.book().best_bid_level(), nullptr);
    EXPECT_DOUBLE_EQ(engine.book().best_bid_level()->total_qty.to_double(), 5.0);

    // 1 Taker trade + 1 Maker trade + 1 ACK = 3 reports
    ASSERT_EQ(ctx.report_count, 3);
    EXPECT_DOUBLE_EQ(ctx.reports[1].remaining_qty.to_double(), 0.0); // Taker remaining
    EXPECT_DOUBLE_EQ(ctx.reports[2].remaining_qty.to_double(), 5.0); // Maker remaining
}

TEST(MatchingEngineTest, MultiLevelSweep) {
    MatchingEngine engine;
    ExecutionContext ctx;

    // Asks: 10 @ 100.50, 20 @ 100.60
    engine.submit_order(make_order(1, 100.50, 10.0, Side::Sell, OrderType::Limit, TimeInForce::GTC, 1), ctx);
    engine.submit_order(make_order(2, 100.60, 20.0, Side::Sell, OrderType::Limit, TimeInForce::GTC, 2), ctx);
    ctx.reset();

    // Aggressive Bid: 25 shares @ 100.70 (sweeps level 1 fully, level 2 partially)
    Order o3 = make_order(3, 100.70, 25.0, Side::Buy, OrderType::Limit, TimeInForce::GTC, 3);
    MatchingResult res = engine.submit_order(o3, ctx);

    EXPECT_EQ(res.status, MatchingStatus::Filled);
    EXPECT_EQ(res.trades_generated, 2);

    // Ask book best ask should be 100.60 with 5 remaining
    EXPECT_DOUBLE_EQ(engine.book().best_ask().to_double(), 100.60);
    EXPECT_DOUBLE_EQ(engine.book().best_ask_level()->total_qty.to_double(), 5.0);
}

TEST(MatchingEngineTest, FIFOOrderingPriority) {
    MatchingEngine engine;
    ExecutionContext ctx;

    // Resting Bids: o1 (10 @ 100.50, seq 1), o2 (10 @ 100.50, seq 2)
    engine.submit_order(make_order(1, 100.50, 10.0, Side::Buy, OrderType::Limit, TimeInForce::GTC, 1), ctx);
    engine.submit_order(make_order(2, 100.50, 10.0, Side::Buy, OrderType::Limit, TimeInForce::GTC, 2), ctx);
    ctx.reset();

    // Aggressive Ask: 12 @ 100.50
    Order o3 = make_order(3, 100.50, 12.0, Side::Sell, OrderType::Limit, TimeInForce::GTC, 3);
    engine.submit_order(o3, ctx);

    // o1 should be fully matched first, o2 partially matched (2 units filled, 8 remaining)
    ASSERT_EQ(ctx.trade_count, 2);
    EXPECT_EQ(ctx.trades[0].sell_order_id, OrderId(3));
    EXPECT_EQ(ctx.trades[0].buy_order_id, OrderId(1)); // First in queue
    EXPECT_DOUBLE_EQ(ctx.trades[0].qty.to_double(), 10.0);

    EXPECT_EQ(ctx.trades[1].buy_order_id, OrderId(2)); // Second in queue
    EXPECT_DOUBLE_EQ(ctx.trades[1].qty.to_double(), 2.0);
}

TEST(MatchingEngineTest, MarketOrderSweepAndExpire) {
    MatchingEngine engine;
    ExecutionContext ctx;

    // Resting Asks: 10 @ 100.50
    engine.submit_order(make_order(1, 100.50, 10.0, Side::Sell, OrderType::Limit, TimeInForce::GTC, 1), ctx);
    ctx.reset();

    // Aggressive Market Buy: 15 shares (sweeps resting 10, remaining 5 expires)
    Order o2 = make_order(2, 0.0, 15.0, Side::Buy, OrderType::Market, TimeInForce::IOC, 2);
    MatchingResult res = engine.submit_order(o2, ctx);

    EXPECT_EQ(res.status, MatchingStatus::Expired);
    EXPECT_EQ(res.trades_generated, 1);

    // Expiration cancellation report emitted for taker balance
    ASSERT_EQ(ctx.report_count, 4); // 1 ACK + 1 Taker Trade + 1 Maker Trade + 1 Expiration Cancel
    EXPECT_EQ(ctx.reports[3].exec_type, ExecutionType::Cancel);
    EXPECT_DOUBLE_EQ(ctx.reports[3].last_qty.to_double(), 0.0);
}

TEST(MatchingEngineTest, OrderCancellation) {
    MatchingEngine engine;
    ExecutionContext ctx;

    engine.submit_order(make_order(1, 100.50, 10.0, Side::Buy, OrderType::Limit, TimeInForce::GTC, 1), ctx);
    ctx.reset();

    bool res = engine.cancel_order(OrderId(1), ClientId(1), AccountId(1), SymbolId(1), ctx);
    EXPECT_TRUE(res);
    EXPECT_EQ(engine.book().best_bid().to_double(), 0.0);

    ASSERT_EQ(ctx.report_count, 1);
    EXPECT_EQ(ctx.reports[0].exec_type, ExecutionType::Cancel);
}

TEST(MatchingEngineTest, ReplayDeterminism) {
    MatchingEngine engine1;
    MatchingEngine engine2;
    ExecutionContext ctx1;
    ExecutionContext ctx2;

    std::vector<Order> event_log = {
        make_order(1, 100.50, 10.0, Side::Buy, OrderType::Limit, TimeInForce::GTC, 1),
        make_order(2, 100.60, 5.0, Side::Sell, OrderType::Limit, TimeInForce::GTC, 2),
        make_order(3, 100.50, 12.0, Side::Sell, OrderType::Limit, TimeInForce::GTC, 3),
        make_order(4, 100.40, 20.0, Side::Buy, OrderType::Limit, TimeInForce::GTC, 4)
    };

    for (const auto& ord : event_log) {
        engine1.submit_order(ord, ctx1);
    }

    for (const auto& ord : event_log) {
        engine2.submit_order(ord, ctx2);
    }

    // Verify both engines resulted in identical Bid/Ask spreads and matched metrics
    EXPECT_DOUBLE_EQ(engine1.book().best_bid().to_double(), engine2.book().best_bid().to_double());
    EXPECT_DOUBLE_EQ(engine1.book().best_ask().to_double(), engine2.book().best_ask().to_double());
    EXPECT_EQ(engine1.stats().trades_executed, engine2.stats().trades_executed);
    EXPECT_EQ(engine1.stats().shares_executed, engine2.stats().shares_executed);
}

TEST(MatchingEngineTest, ExchangeSimulatorSymbolRouting) {
    ExchangeSimulator exchange;
    SymbolId sym1(1);
    SymbolId sym2(2);

    auto reg1 = exchange.register_symbol(sym1);
    auto reg2 = exchange.register_symbol(sym2);

    ASSERT_TRUE(reg1.has_value());
    ASSERT_TRUE(reg2.has_value());

    MatchingEngine* engine_btc = exchange.get_engine(sym1);
    MatchingEngine* engine_eth = exchange.get_engine(sym2);

    ASSERT_NE(engine_btc, nullptr);
    ASSERT_NE(engine_eth, nullptr);

    ExecutionContext ctx;
    Order o1 = make_order(1, 100.50, 10.0, Side::Buy, OrderType::Limit, TimeInForce::GTC, 1);
    // Submit to BTC engine
    engine_btc->submit_order(o1, ctx);

    EXPECT_DOUBLE_EQ(engine_btc->book().best_bid().to_double(), 100.50);
    EXPECT_DOUBLE_EQ(engine_eth->book().best_bid().to_double(), 0.0); // ETH remains untouched
}
