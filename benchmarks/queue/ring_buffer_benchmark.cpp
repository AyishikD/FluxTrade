#include <benchmark/benchmark.h>
#include <common/ring_buffer.hpp>
#include <common/object_pool.hpp>
#include <common/thread_pinning.hpp>
#include <kernel/event_bus.hpp>
#include <thread>
#include <atomic>
#include <chrono>

using namespace fluxtrade;

struct Token {
    uint64_t val;
};

// SPSC Queue Cross-Thread Round-Trip Ping-Pong Latency
static void BM_SPSC_CrossThread_Latency(benchmark::State& state) {
    // Two queues for round trip
    auto q1 = std::make_unique<SPSCQueue<Token, 1024>>();
    auto q2 = std::make_unique<SPSCQueue<Token, 1024>>();
    
    std::atomic<bool> running{true};
    
    // Spawn consumer thread
    std::thread consumer([&]() {
        pin_thread(1); // Pin consumer to Core 1
        Token tk;
        while (running.load(std::memory_order_relaxed)) {
            if (q1->pop(tk)) {
                // Echo back to q2
                while (!q2->push(tk) && running.load(std::memory_order_relaxed)) {
                    std::this_thread::yield();
                }
            } else {
                std::this_thread::yield();
            }
        }
    });

    pin_thread(2); // Pin producer/benchmark thread to Core 2
    Token tk{42};
    Token reply;

    for (auto _ : state) {
        // Enqueue token to consumer
        while (!q1->push(tk)) {
            std::this_thread::yield();
        }
        
        // Wait for reply from consumer
        while (!q2->pop(reply)) {
            std::this_thread::yield();
        }
        
        benchmark::DoNotOptimize(reply);
    }

    running.store(false, std::memory_order_relaxed);
    consumer.join();
    unbind();
}
// Run this benchmark to measure time per round-trip exchange.
// Divide by 2 in analysis to get one-way queue crossing time.
BENCHMARK(BM_SPSC_CrossThread_Latency)->Unit(benchmark::kNanosecond);


// MPSC Queue Multi-Producer Single-Consumer Round-Trip Latency
static void BM_MPSC_CrossThread_Latency(benchmark::State& state) {
    auto q_in = std::make_unique<MPSCQueue<Token, 1024>>();
    auto q_out = std::make_unique<SPSCQueue<Token, 1024>>();
    
    std::atomic<bool> running{true};
    
    // Spawn consumer thread
    std::thread consumer([&]() {
        pin_thread(1);
        Token tk;
        while (running.load(std::memory_order_relaxed)) {
            if (q_in->pop(tk)) {
                while (!q_out->push(tk) && running.load(std::memory_order_relaxed)) {
                    std::this_thread::yield();
                }
            } else {
                std::this_thread::yield();
            }
        }
    });

    pin_thread(2);
    Token tk{100};
    Token reply;

    for (auto _ : state) {
        while (!q_in->push(tk)) {
            std::this_thread::yield();
        }
        while (!q_out->pop(reply)) {
            std::this_thread::yield();
        }
        benchmark::DoNotOptimize(reply);
    }

    running.store(false, std::memory_order_relaxed);
    consumer.join();
    unbind();
}
BENCHMARK(BM_MPSC_CrossThread_Latency)->Unit(benchmark::kNanosecond);


struct OrderPayload {
    uint64_t order_id;
    uint32_t symbol_id;
    uint64_t price;
    uint64_t quantity;
    bool is_active;
    
    OrderPayload(uint64_t id, uint32_t sym, uint64_t pr, uint64_t qty)
        : order_id(id), symbol_id(sym), price(pr), quantity(qty), is_active(true) {}
};

// Benchmark standard heap allocation lifecycle (new + construct + destruct + delete)
static void BM_Heap_Lifecycle(benchmark::State& state) {
    uint64_t i = 0;
    for (auto _ : state) {
        auto* ptr = new OrderPayload(i++, 1, 10000, 50);
        benchmark::DoNotOptimize(ptr);
        delete ptr;
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Heap_Lifecycle);

// Benchmark ObjectPool allocation lifecycle (allocate + placement new + destruct + deallocate)
static void BM_ObjectPool_Lifecycle(benchmark::State& state) {
    ObjectPool<OrderPayload, 1024> pool;
    uint64_t i = 0;
    for (auto _ : state) {
        auto* ptr = pool.allocate(i++, 1, 10000, 50);
        benchmark::DoNotOptimize(ptr);
        pool.deallocate(ptr);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ObjectPool_Lifecycle);

// Benchmark EventBus single subscriber publish latency
static void BM_EventBus_SingleSubscriber(benchmark::State& state) {
    EventBus<Token> bus;
    uint64_t counter = 0;
    bus.subscribe<Token>([&counter](const Token& event) {
        counter += event.val;
    });

    Token tk{1};
    for (auto _ : state) {
        bus.publish(tk);
        benchmark::DoNotOptimize(counter);
    }
}
BENCHMARK(BM_EventBus_SingleSubscriber);

// Benchmark EventBus multi subscriber (5 subscribers) publish latency
static void BM_EventBus_MultiSubscriber(benchmark::State& state) {
    EventBus<Token> bus;
    uint64_t counter = 0;
    
    for (int i = 0; i < 5; ++i) {
        bus.subscribe<Token>([&counter](const Token& event) {
            counter += event.val;
        });
    }

    Token tk{1};
    for (auto _ : state) {
        bus.publish(tk);
        benchmark::DoNotOptimize(counter);
    }
}
BENCHMARK(BM_EventBus_MultiSubscriber);

BENCHMARK_MAIN();
