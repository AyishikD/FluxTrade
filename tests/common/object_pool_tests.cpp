#include <gtest/gtest.h>
#include <common/object_pool.hpp>
#include <string>

using namespace fluxtrade;

struct Dummy {
    int x;
    std::string s;
    static int active_instances;

    Dummy(int x_val, std::string s_val) : x(x_val), s(std::move(s_val)) {
        active_instances++;
    }
    ~Dummy() {
        active_instances--;
    }
};

int Dummy::active_instances = 0;

TEST(ObjectPoolTest, AllocateDeallocate) {
    Dummy::active_instances = 0;
    {
        ObjectPool<Dummy, 4> pool;
        EXPECT_EQ(pool.available(), 4);

        Dummy* d1 = pool.allocate(42, "hello");
        EXPECT_NE(d1, nullptr);
        EXPECT_EQ(d1->x, 42);
        EXPECT_EQ(d1->s, "hello");
        EXPECT_EQ(pool.available(), 3);
        EXPECT_EQ(Dummy::active_instances, 1);

        Dummy* d2 = pool.allocate(100, "world");
        EXPECT_EQ(pool.available(), 2);
        EXPECT_EQ(Dummy::active_instances, 2);

        pool.deallocate(d1);
        EXPECT_EQ(pool.available(), 3);
        EXPECT_EQ(Dummy::active_instances, 1);

        pool.deallocate(d2);
        EXPECT_EQ(pool.available(), 4);
        EXPECT_EQ(Dummy::active_instances, 0);
    }
}

TEST(ObjectPoolTest, PoolExhaustion) {
    ObjectPool<int, 2> pool;
    int* p1 = pool.allocate(10);
    int* p2 = pool.allocate(20);
    EXPECT_EQ(pool.available(), 0);

    EXPECT_THROW(pool.allocate(30), std::bad_alloc);

    pool.deallocate(p1);
    EXPECT_EQ(pool.available(), 1);
    int* p3 = pool.allocate(30);
    EXPECT_EQ(*p3, 30);
    pool.deallocate(p2);
    pool.deallocate(p3);
}
