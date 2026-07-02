#include <benchmark/benchmark.h>
#include <events/event_header.hpp>
#include <events/order_events.hpp>
#include <events/serialization.hpp>
#include <vector>
#include <cstring>

using namespace fluxtrade;

// Benchmark simple event construction
static void BM_Event_Construction(benchmark::State& state) {
    uint64_t i = 0;
    for (auto _ : state) {
        OrderReceivedEvent event;
        event.header.timestamp = i++;
        event.header.seq_num = i;
        event.order_id = i;
        benchmark::DoNotOptimize(event);
    }
}
BENCHMARK(BM_Event_Construction);

// Benchmark event copy operations
static void BM_Event_Copy(benchmark::State& state) {
    OrderReceivedEvent event;
    event.header.timestamp = 12345ULL;
    event.order_id = 999ULL;

    OrderReceivedEvent destination;
    for (auto _ : state) {
        destination = event;
        benchmark::DoNotOptimize(destination);
    }
}
BENCHMARK(BM_Event_Copy);

// Benchmark header member access latency
static void BM_Event_HeaderAccess(benchmark::State& state) {
    OrderReceivedEvent event;
    event.header.timestamp = 9999ULL;
    uint64_t val = 0;

    for (auto _ : state) {
        val = event.header.timestamp;
        benchmark::DoNotOptimize(val);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Event_HeaderAccess);

// Benchmark binary serialization
static void BM_Event_Serialization(benchmark::State& state) {
    OrderReceivedEvent event;
    event.header.timestamp = 12345ULL;
    event.header.seq_num = 1ULL;
    event.order_id = 555ULL;

    uint8_t buffer[128];
    for (auto _ : state) {
        size_t size = serialize(event, buffer, sizeof(buffer));
        benchmark::DoNotOptimize(size);
        benchmark::DoNotOptimize(buffer);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Event_Serialization);

// Benchmark binary deserialization
static void BM_Event_Deserialization(benchmark::State& state) {
    OrderReceivedEvent event;
    event.header.timestamp = 12345ULL;
    event.order_id = 555ULL;

    uint8_t buffer[128];
    (void)serialize(event, buffer, sizeof(buffer));

    OrderReceivedEvent destination;
    for (auto _ : state) {
        bool success = deserialize(buffer, sizeof(OrderReceivedEvent), destination);
        benchmark::DoNotOptimize(success);
        benchmark::DoNotOptimize(destination);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Event_Deserialization);

BENCHMARK_MAIN();
