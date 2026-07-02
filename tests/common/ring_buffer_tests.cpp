#include <gtest/gtest.h>
#include <common/ring_buffer.hpp>
#include <thread>
#include <vector>

using namespace fluxtrade;

TEST(RingBufferTest, BasicPushPop) {
    RingBuffer<int, 4> queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);

    EXPECT_TRUE(queue.push(10));
    EXPECT_TRUE(queue.push(20));
    EXPECT_EQ(queue.size(), 2);
    EXPECT_FALSE(queue.empty());

    int val = 0;
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 10);
    EXPECT_EQ(queue.size(), 1);

    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 20);
    EXPECT_TRUE(queue.empty());
}

TEST(RingBufferTest, CapacityLimits) {
    RingBuffer<int, 2> queue;
    EXPECT_TRUE(queue.push(1));
    EXPECT_TRUE(queue.push(2));
    EXPECT_FALSE(queue.push(3)); // Full

    int val = 0;
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 1);

    EXPECT_TRUE(queue.push(3)); // Emptied one slot
    EXPECT_FALSE(queue.push(4)); // Full again
}

TEST(RingBufferTest, MultiThreadedSPSC) {
    constexpr size_t NUM_ITEMS = 100'000;
    RingBuffer<size_t, 1024> queue;

    std::thread producer([&]() {
        for (size_t i = 1; i <= NUM_ITEMS; ++i) {
            while (!queue.push(i)) {
                std::this_thread::yield();
            }
        }
    });

    std::thread consumer([&]() {
        size_t expected = 1;
        size_t popped_val = 0;
        while (expected <= NUM_ITEMS) {
            if (queue.pop(popped_val)) {
                EXPECT_EQ(popped_val, expected);
                expected++;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();
}

TEST(MPSCQueueTest, BasicMPSC) {
    MPSCQueue<int, 4> queue;
    EXPECT_TRUE(queue.empty());

    EXPECT_TRUE(queue.push(1));
    EXPECT_TRUE(queue.push(2));
    int val = 0;
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 1);
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 2);
}

TEST(MPSCQueueTest, MultiProducerSingleConsumer) {
    constexpr size_t NUM_PRODUCERS = 4;
    constexpr size_t ITEMS_PER_PRODUCER = 25'000;
    constexpr size_t TOTAL_ITEMS = NUM_PRODUCERS * ITEMS_PER_PRODUCER;

    MPSCQueue<size_t, 2048> queue;
    std::vector<std::thread> producers;
    producers.reserve(NUM_PRODUCERS);

    for (size_t p = 0; p < NUM_PRODUCERS; ++p) {
        producers.emplace_back([&queue]() {
            for (size_t i = 0; i < ITEMS_PER_PRODUCER; ++i) {
                while (!queue.push(1)) {
                    std::this_thread::yield();
                }
            }
        });
    }

    std::atomic<size_t> popped_count{0};
    std::thread consumer([&queue, &popped_count]() {
        size_t val = 0;
        while (popped_count.load(std::memory_order_relaxed) < TOTAL_ITEMS) {
            if (queue.pop(val)) {
                popped_count.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
    });

    for (auto& producer : producers) {
        producer.join();
    }
    consumer.join();

    EXPECT_EQ(popped_count.load(), TOTAL_ITEMS);
}
