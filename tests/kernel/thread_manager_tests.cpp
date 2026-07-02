#include <gtest/gtest.h>
#include <kernel/thread_manager.hpp>
#include <kernel/clock.hpp>
#include <kernel/service_registry.hpp>
#include <atomic>

using namespace fluxtrade;

TEST(ThreadManagerTest, SpawnAndJoin) {
    ThreadManager tm;
    std::atomic<bool> flag{false};

    auto tid_res = tm.spawn_worker("test_worker", 0, [&flag]() {
        flag.store(true);
    });

    ASSERT_TRUE(tid_res.has_value());
    tm.join_all();

    EXPECT_TRUE(flag.load());
    auto stats = tm.get_stats();
    ASSERT_EQ(stats.size(), 1);
    EXPECT_EQ(stats[0].name, "test_worker");
    EXPECT_FALSE(stats[0].is_active);
}

TEST(ClockTest, HighResAndCycles) {
    Timestamp t1 = Clock::now_steady();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    Timestamp t2 = Clock::now_steady();
    EXPECT_GT(t2, t1);

    uint64_t c1 = Clock::now_cycles();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    uint64_t c2 = Clock::now_cycles();
    // On architectures with working TSC, cycle count increases
    EXPECT_GE(c2, c1);
}

struct TestService {
    int value = 42;
};

TEST(ServiceRegistryTest, RegisterAndGet) {
    ServiceRegistry registry;
    TestService service;

    registry.register_service(&service);

    auto* fetched = registry.get_service<TestService>();
    ASSERT_NE(fetched, nullptr);
    EXPECT_EQ(fetched->value, 42);

    struct NonExistentService {};
    EXPECT_EQ(registry.get_service<NonExistentService>(), nullptr);
}
