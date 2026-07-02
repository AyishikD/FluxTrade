#include <benchmark/benchmark.h>
#include <matching/matching_engine.hpp>

using namespace fluxtrade;

// Helper to construct standard Order frames
Order make_bench_order(
    uint64_t id,
    double price,
    double qty,
    Side side,
    OrderType type = OrderType::Limit
) {
    return Order{
        OrderId(id),
        Price::from_double(price),
        Quantity::from_double(qty),
        id,
        1000,
        1002,
        ClientId(1),
        AccountId(1),
        SymbolId(1),
        side,
        type,
        TimeInForce::GTC,
        OrderFlags::None
    };
}

// Benchmark resting limit order insertion (no crosses)
static void BM_MatchingEngine_RestingLimit(benchmark::State& state) {
    MatchingEngine engine;
    ExecutionContext ctx;
    uint64_t next_id = 1;

    for (auto _ : state) {
        if (next_id >= 10000) {
            engine.reset();
            next_id = 1;
        }
        Order o = make_bench_order(next_id++, 100.50, 10.0, Side::Buy);
        ctx.reset();

        MatchingResult res = engine.submit_order(o, ctx);
        benchmark::DoNotOptimize(res);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_MatchingEngine_RestingLimit);

// Benchmark crossing limit order matches (1 trade execution)
static void BM_MatchingEngine_CrossingLimit(benchmark::State& state) {
    MatchingEngine engine;
    ExecutionContext ctx;
    uint64_t next_id = 1;

    for (auto _ : state) {
        if (next_id >= 10000) {
            engine.reset();
            next_id = 1;
        }
        // Insert resting ask
        Order resting = make_bench_order(next_id++, 100.50, 10.0, Side::Sell);
        engine.submit_order(resting, ctx);

        // Cross with aggressive bid
        Order aggressive = make_bench_order(next_id++, 100.50, 10.0, Side::Buy);
        ctx.reset();

        MatchingResult res = engine.submit_order(aggressive, ctx);
        benchmark::DoNotOptimize(res);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_MatchingEngine_CrossingLimit);

// Benchmark multi-level sweep matches (sweeping 5 distinct resting levels)
static void BM_MatchingEngine_MultiLevelSweep(benchmark::State& state) {
    MatchingEngine engine;
    ExecutionContext ctx;
    uint64_t next_id = 1;

    for (auto _ : state) {
        if (next_id >= 5000) {
            engine.reset();
            next_id = 1;
        }
        // Insert 5 resting asks at 100.10, 100.20, 100.30, 100.40, 100.50
        for (int i = 1; i <= 5; ++i) {
            Order resting = make_bench_order(next_id++, 100.0 + (i * 0.10), 10.0, Side::Sell);
            engine.submit_order(resting, ctx);
        }

        // Aggressive bid sweeps all 5 levels (50 shares @ 101.00)
        Order aggressive = make_bench_order(next_id++, 101.00, 50.0, Side::Buy);
        ctx.reset();

        MatchingResult res = engine.submit_order(aggressive, ctx);
        benchmark::DoNotOptimize(res);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_MatchingEngine_MultiLevelSweep);

// Benchmark order cancellation via the Matching Engine
static void BM_MatchingEngine_CancelOrder(benchmark::State& state) {
    MatchingEngine engine;
    ExecutionContext ctx;
    uint64_t next_id = 1;

    for (auto _ : state) {
        if (next_id >= 10000) {
            engine.reset();
            next_id = 1;
        }
        Order o = make_bench_order(next_id++, 100.50, 10.0, Side::Buy);
        engine.submit_order(o, ctx);

        ctx.reset();
        bool res = engine.cancel_order(OrderId(next_id - 1), ClientId(1), AccountId(1), SymbolId(1), ctx);
        benchmark::DoNotOptimize(res);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_MatchingEngine_CancelOrder);

BENCHMARK_MAIN();
