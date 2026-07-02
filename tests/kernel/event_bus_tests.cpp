#include <gtest/gtest.h>
#include <kernel/event_bus.hpp>

using namespace fluxtrade;

struct TestEventA {
    int value;
};

struct TestEventB {
    std::string text;
};

using TestEventBus = EventBus<TestEventA, TestEventB>;

TEST(EventBusTest, SingleSubscriber) {
    TestEventBus bus;
    int received_value = 0;

    bus.subscribe<TestEventA>([&received_value](const TestEventA& event) {
        received_value = event.value;
    });

    TestEventA ev{100};
    bus.publish(ev);

    EXPECT_EQ(received_value, 100);
}

TEST(EventBusTest, MultipleSubscribers) {
    TestEventBus bus;
    int sum = 0;

    bus.subscribe<TestEventA>([&sum](const TestEventA& event) {
        sum += event.value;
    });

    bus.subscribe<TestEventA>([&sum](const TestEventA& event) {
        sum += event.value * 2;
    });

    TestEventA ev{10};
    bus.publish(ev);

    // 10 + 20 = 30
    EXPECT_EQ(sum, 30);
}

TEST(EventBusTest, EventIsolation) {
    TestEventBus bus;
    int val_a = 0;
    std::string text_b = "";

    bus.subscribe<TestEventA>([&val_a](const TestEventA& event) {
        val_a = event.value;
    });

    bus.subscribe<TestEventB>([&text_b](const TestEventB& event) {
        text_b = event.text;
    });

    TestEventB ev_b{"hello"};
    bus.publish(ev_b);

    // Event A should not be triggered, Event B should be triggered
    EXPECT_EQ(val_a, 0);
    EXPECT_EQ(text_b, "hello");
}
