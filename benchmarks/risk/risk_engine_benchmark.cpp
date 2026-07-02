#include <benchmark/benchmark.h>
#include <risk/risk_engine.hpp>

using namespace fluxtrade;

// Helper to construct a standard RiskContext frame
RiskContext make_bench_context(
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

// Benchmark Credit Check (O(1) buying power calculation)
static void BM_RiskEngine_CreditCheck(benchmark::State& state) {
    RiskContext ctx = make_bench_context(1, 100.50, 10.0, 1, 1);
    RiskLimits limits;
    limits.max_daily_exposure = 2000000000ULL;
    
    AccountState account;
    account.available_buying_power = 1000000000ULL;
    account.daily_exposure = 0;

    for (auto _ : state) {
        bool ok = CreditManager::validate(ctx, limits, account, PositionState{});
        benchmark::DoNotOptimize(ok);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_RiskEngine_CreditCheck);

// Benchmark Position Check (O(1) net long/short limits)
static void BM_RiskEngine_PositionCheck(benchmark::State& state) {
    RiskContext ctx = make_bench_context(1, 100.50, 10.0, 1, 1);
    RiskLimits limits;
    
    PositionState position;
    position.net_qty = 0;
    position.max_long_limit = 500000ULL;
    position.max_short_limit = 500000ULL;

    for (auto _ : state) {
        bool ok = MarginManager::validate(ctx, limits, AccountState{}, position);
        benchmark::DoNotOptimize(ok);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_RiskEngine_PositionCheck);

// Benchmark STP Check (Compile-time adapter validation)
static void BM_RiskEngine_STPCheck(benchmark::State& state) {
    RiskContext ctx = make_bench_context(1, 100.50, 10.0, 1, 1);
    RiskLimits limits;

    for (auto _ : state) {
        bool ok = STPValidator::validate(ctx, limits, AccountState{}, PositionState{});
        benchmark::DoNotOptimize(ok);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_RiskEngine_STPCheck);

// Benchmark Price Band Check (Deviation validation)
static void BM_RiskEngine_PriceBandCheck(benchmark::State& state) {
    RiskContext ctx = make_bench_context(1, 100.50, 10.0, 1, 1, Side::Buy, 100.00);
    RiskLimits limits;
    limits.price_band_tolerance = 0.10;

    for (auto _ : state) {
        bool ok = PriceBandValidator::validate(ctx, limits, AccountState{}, PositionState{});
        benchmark::DoNotOptimize(ok);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_RiskEngine_PriceBandCheck);

// Benchmark Kill Switch Check (Atomic read operations)
static void BM_RiskEngine_KillSwitchCheck(benchmark::State& state) {
    RiskContext ctx = make_bench_context(1, 100.50, 10.0, 1, 1);
    RiskLimits limits;
    limits.kill_switch_active = false;

    for (auto _ : state) {
        bool ok = KillSwitch::validate(ctx, limits, AccountState{}, PositionState{});
        benchmark::DoNotOptimize(ok);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_RiskEngine_KillSwitchCheck);

// Benchmark Entire Validation Pipeline (Sequencing all validators)
static void BM_RiskEngine_FullPipeline(benchmark::State& state) {
    RiskEngine engine;
    // Set max orders per sec to 0 to bypass rate limit checks during raw speed measurement
    engine.limits().max_orders_per_sec = 0;
    RiskTrace trace;
    uint64_t next_id = 1;

    for (auto _ : state) {
        RiskContext ctx = make_bench_context(next_id, 100.50, 10.0, 1, 1, Side::Buy, 100.00, next_id);
        next_id++;
        trace.stage_count = 0;

        auto res = engine.validate(ctx, trace);
        benchmark::DoNotOptimize(res);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_RiskEngine_FullPipeline);

BENCHMARK_MAIN();
