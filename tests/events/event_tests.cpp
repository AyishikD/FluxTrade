#include <gtest/gtest.h>
#include <events/event_header.hpp>
#include <events/event_traits.hpp>
#include <events/base_event.hpp>
#include <events/order_events.hpp>
#include <events/trade_events.hpp>
#include <events/market_events.hpp>
#include <events/risk_events.hpp>
#include <events/system_events.hpp>
#include <events/serialization.hpp>

using namespace fluxtrade;

// 1. Static Layout and Constraints Verification
TEST(EventsTest, StaticConstraints) {
    // Header size and alignment
    static_assert(sizeof(EventHeader) == 40);
    static_assert(alignof(EventHeader) == 8);

    // Concept compliance
    static_assert(ExchangeEvent<OrderReceivedEvent>);
    static_assert(ExchangeEvent<OrderAcceptedEvent>);
    static_assert(ExchangeEvent<OrderRejectedEvent>);
    static_assert(ExchangeEvent<TradeExecutedEvent>);
    static_assert(ExchangeEvent<BookUpdatedEvent>);
    static_assert(ExchangeEvent<HeartbeatEvent>);

    // Size constraints for POD optimization
    static_assert(sizeof(OrderReceivedEvent) == 72);
    static_assert(sizeof(OrderAcceptedEvent) == 72);
    static_assert(sizeof(OrderRejectedEvent) == 56);
    static_assert(sizeof(TradeExecutedEvent) == 72);
    static_assert(sizeof(BookUpdatedEvent) == 64);
    static_assert(sizeof(HeartbeatEvent) == 40);

    SUCCEED();
}

// 2. Trait Category Verification
TEST(EventsTest, TraitsAndConcepts) {
    EXPECT_TRUE(is_order_event_v<OrderReceivedEvent>);
    EXPECT_TRUE(is_order_event_v<OrderAcceptedEvent>);
    EXPECT_FALSE(is_order_event_v<TradeExecutedEvent>);

    EXPECT_TRUE(is_trade_event_v<TradeExecutedEvent>);
    EXPECT_FALSE(is_trade_event_v<OrderReceivedEvent>);

    EXPECT_TRUE(is_market_event_v<BookUpdatedEvent>);
    EXPECT_TRUE(is_system_event_v<HeartbeatEvent>);
    EXPECT_TRUE(is_replay_event_v<ReplayStartedEvent>);
}

// 3. Serialization and Deserialization Correctness
TEST(EventsTest, BinarySerializationRoundtrip) {
    OrderReceivedEvent event;
    event.header.timestamp = 123456789ULL;
    event.header.seq_num = 98765ULL;
    event.header.correlation_id = 11111ULL;
    event.header.symbol_id = 42;
    event.header.id = EventId::OrderReceived;
    event.header.source = ModuleId::Network;
    event.header.version = ProtocolVersion::Current;
    
    event.order_id = 999999ULL;
    event.client_id = 7;
    event.price = 1005000ULL; // 100.5000
    event.qty = 500ULL;
    event.side = Side::Sell;

    // Allocate buffer
    uint8_t buffer[128];
    std::memset(buffer, 0, sizeof(buffer));

    // Serialize
    size_t size = serialize(event, buffer, sizeof(buffer));
    EXPECT_EQ(size, sizeof(OrderReceivedEvent));

    // Deserialize
    OrderReceivedEvent deserialized;
    bool success = deserialize(buffer, size, deserialized);
    EXPECT_TRUE(success);

    // Verify all header elements
    EXPECT_EQ(deserialized.header.timestamp, event.header.timestamp);
    EXPECT_EQ(deserialized.header.seq_num, event.header.seq_num);
    EXPECT_EQ(deserialized.header.correlation_id, event.header.correlation_id);
    EXPECT_EQ(deserialized.header.symbol_id, event.header.symbol_id);
    EXPECT_EQ(deserialized.header.id, event.header.id);
    EXPECT_EQ(deserialized.header.source, event.header.source);
    EXPECT_EQ(deserialized.header.version, event.header.version);

    // Verify payload
    EXPECT_EQ(deserialized.order_id, event.order_id);
    EXPECT_EQ(deserialized.client_id, event.client_id);
    EXPECT_EQ(deserialized.price, event.price);
    EXPECT_EQ(deserialized.qty, event.qty);
    EXPECT_EQ(deserialized.side, event.side);
}

// 4. Bounded Buffer Boundary Checks
TEST(EventsTest, BoundedBufferChecks) {
    OrderReceivedEvent event;
    uint8_t buffer[32]; // Too small (needs 72 bytes)

    size_t size = serialize(event, buffer, sizeof(buffer));
    EXPECT_EQ(size, 0);

    OrderReceivedEvent deserialized;
    bool success = deserialize(buffer, sizeof(buffer), deserialized);
    EXPECT_FALSE(success);
}
