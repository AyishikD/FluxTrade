#include <benchmark/benchmark.h>
#include <matching/order_book/order_book.hpp>

using namespace fluxtrade;

// Benchmark order insertion at an existing price level (O(1) appending)
static void BM_OrderBook_InsertExistingLevel(benchmark::State& state) {
    LimitOrderBook book;
    
    // Create base level
    Order base_order{
        OrderId(1), Price::from_double(100.50), Quantity::from_double(10),
        1, 1000, 1001, ClientId(1), AccountId(1), SymbolId(1),
        Side::Buy, OrderType::Limit, TimeInForce::GTC
    };
    book.insert(base_order, 1);

    uint64_t next_id = 2;
    for (auto _ : state) {
        if (next_id >= 10000) {
            book.clear();
            book.insert(base_order, 1);
            next_id = 2;
        }
        Order order{
            OrderId(next_id++), Price::from_double(100.50), Quantity::from_double(5),
            next_id, 1000, 1001, ClientId(1), AccountId(1), SymbolId(1),
            Side::Buy, OrderType::Limit, TimeInForce::GTC
        };
        LiveOrder* res = book.insert(order, next_id);
        benchmark::DoNotOptimize(res);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_OrderBook_InsertExistingLevel);

// Benchmark order insertion at a new price level (O(log L) tree lookup/indexing)
static void BM_OrderBook_InsertNewLevel(benchmark::State& state) {
    LimitOrderBook book;
    uint64_t next_id = 1;
    double price_offset = 100.0;

    for (auto _ : state) {
        if (next_id >= 1000) {
            book.clear();
            next_id = 1;
        }
        Order order{
            OrderId(next_id++), Price::from_double(price_offset + (next_id * 0.01)), Quantity::from_double(10),
            next_id, 1000, 1001, ClientId(1), AccountId(1), SymbolId(1),
            Side::Buy, OrderType::Limit, TimeInForce::GTC
        };
        LiveOrder* res = book.insert(order, next_id);
        benchmark::DoNotOptimize(res);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_OrderBook_InsertNewLevel);

// Benchmark order cancellation (O(1) lookup + list removal)
static void BM_OrderBook_CancelOrder(benchmark::State& state) {
    LimitOrderBook book;
    uint64_t next_id = 1;

    for (auto _ : state) {
        if (next_id >= 10000) {
            book.clear();
            next_id = 1;
        }
        Order order{
            OrderId(next_id), Price::from_double(100.50), Quantity::from_double(10),
            next_id, 1000, 1001, ClientId(1), AccountId(1), SymbolId(1),
            Side::Buy, OrderType::Limit, TimeInForce::GTC
        };
        book.insert(order, next_id);

        bool res = book.cancel(OrderId(next_id));
        benchmark::DoNotOptimize(res);
        benchmark::ClobberMemory();
        next_id++;
    }
}
BENCHMARK(BM_OrderBook_CancelOrder);

// Benchmark order quantity modification
static void BM_OrderBook_ModifyQty(benchmark::State& state) {
    LimitOrderBook book;
    Order order{
        OrderId(1), Price::from_double(100.50), Quantity::from_double(10),
        1, 1000, 1001, ClientId(1), AccountId(1), SymbolId(1),
        Side::Buy, OrderType::Limit, TimeInForce::GTC
    };
    book.insert(order, 1);

    uint64_t modifier = 1;
    for (auto _ : state) {
        // Toggle quantity between 5 and 15
        Quantity new_qty = Quantity::from_double((modifier++ % 2 == 0) ? 5.0 : 15.0);
        bool res = book.modify_qty(OrderId(1), new_qty, modifier);
        benchmark::DoNotOptimize(res);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_OrderBook_ModifyQty);

// Benchmark top-of-book lookup (O(1) best bid query)
static void BM_OrderBook_BestBid(benchmark::State& state) {
    LimitOrderBook book;
    Order order{
        OrderId(1), Price::from_double(100.50), Quantity::from_double(10),
        1, 1000, 1001, ClientId(1), AccountId(1), SymbolId(1),
        Side::Buy, OrderType::Limit, TimeInForce::GTC
    };
    book.insert(order, 1);

    Price best_bid;
    for (auto _ : state) {
        best_bid = book.best_bid();
        benchmark::DoNotOptimize(best_bid);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_OrderBook_BestBid);

BENCHMARK_MAIN();
