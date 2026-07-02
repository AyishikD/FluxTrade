#include <benchmark/benchmark.h>
#include <domain/ids.hpp>
#include <domain/price.hpp>
#include <domain/quantity.hpp>
#include <domain/order.hpp>
#include <domain/trade.hpp>
#include <domain/validators.hpp>

using namespace fluxtrade;

// Benchmark Order construction
static void BM_Order_Construction(benchmark::State& state) {
    uint64_t i = 0;
    for (auto _ : state) {
        Order order{
            OrderId(i++),
            Price(1005000000LL),
            Quantity(5000000ULL),
            i, i, i,
            ClientId(1), AccountId(1), SymbolId(1),
            Side::Buy, OrderType::Limit, TimeInForce::GTC, OrderFlags::None
        };
        benchmark::DoNotOptimize(order);
    }
}
BENCHMARK(BM_Order_Construction);

// Benchmark Order copying
static void BM_Order_Copy(benchmark::State& state) {
    Order order{
        OrderId(42), Price(1005000000LL), Quantity(5000000ULL),
        1, 1000, 1002,
        ClientId(1), AccountId(1), SymbolId(1),
        Side::Buy, OrderType::Limit, TimeInForce::GTC, OrderFlags::None
    };

    Order destination;
    for (auto _ : state) {
        destination = order;
        benchmark::DoNotOptimize(destination);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Order_Copy);

// Benchmark Price mathematics (fixed-precision arithmetic)
static void BM_Price_Arithmetic(benchmark::State& state) {
    Price p1(1005000000LL);
    Price p2(500000LL);
    Price result;

    for (auto _ : state) {
        result = p1 + p2;
        benchmark::DoNotOptimize(result);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Price_Arithmetic);

// Benchmark Quantity conversions
static void BM_Quantity_Conversions(benchmark::State& state) {
    double value = 15.65;
    Quantity q;

    for (auto _ : state) {
        q = Quantity::from_double(value);
        benchmark::DoNotOptimize(q);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Quantity_Conversions);

// Benchmark lightweight validation
static void BM_Order_Validation(benchmark::State& state) {
    Order order{
        OrderId(42), Price(1005000000LL), Quantity(5000000ULL),
        1, 1000, 1002,
        ClientId(1), AccountId(1), SymbolId(1),
        Side::Buy, OrderType::Limit, TimeInForce::GTC, OrderFlags::None
    };

    bool valid = false;
    for (auto _ : state) {
        valid = is_valid_price(order.price) && 
                is_valid_quantity(order.qty) && 
                is_valid_side(order.side) && 
                is_valid_order_type(order.type);
        benchmark::DoNotOptimize(valid);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Order_Validation);

BENCHMARK_MAIN();
