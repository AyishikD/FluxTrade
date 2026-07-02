#include <gtest/gtest.h>
#include <exchange/exchange.hpp>
#include <gateway/simulation_transport.hpp>
#include <gateway/packet_encoder.hpp>
#include <gateway/packet_decoder.hpp>
#include <cstring>

using namespace fluxtrade;

class ExchangeTest : public ::testing::Test {
protected:
    void SetUp() override {
        exchange = std::make_unique<Exchange>(&transport);
        // Register Symbol 1 (Apple AAPL)
        exchange->add_symbol(1);
    }

    void TearDown() override {
        exchange->stop();
    }

    SimulationTransport transport;
    std::unique_ptr<Exchange> exchange;
};

TEST_F(ExchangeTest, DispatcherSymbolRouting) {
    OrderCommand cmd1;
    cmd1.symbol_id = SymbolId(1);
    cmd1.client_order_id = 1001;

    OrderCommand cmd2;
    cmd2.symbol_id = SymbolId(2);
    cmd2.client_order_id = 1002;

    (void)exchange->dispatcher().dispatch(cmd1);
    (void)exchange->dispatcher().dispatch(cmd2);

    // Verify routing slot distributions
    auto& q1 = exchange->dispatcher().get_queue(1);
    auto& q2 = exchange->dispatcher().get_queue(2);

    EXPECT_EQ(q1.size(), 1);
    EXPECT_EQ(q2.size(), 1);

    OrderCommand popped;
    ASSERT_TRUE(q1.pop(popped));
    EXPECT_EQ(popped.client_order_id, 1001);
}

TEST_F(ExchangeTest, E2EOrderMatchingFills) {
    // Start Exchange loops
    ASSERT_TRUE(exchange->start("127.0.0.1", 12345));

    // Simulate mock client connection
    transport.simulate_connect(10);

    // 1. Authenticate connection
    (void)exchange->gateway().sessions().authenticate(10, 42, 99, "trader1", "pass");

    // 2. Submit Buy Order (10 shares @ 100.50)
    uint8_t buffer[512];
    NewOrderPacket buy_order{};
    buy_order.client_order_id = 1001;
    buy_order.symbol_id = 1;
    buy_order.account_id = 99;
    buy_order.price_ticks = Price::from_double(100.50).ticks();
    buy_order.qty_units = Quantity::from_double(10.0).units();
    buy_order.side = 0; // Buy
    buy_order.type = 0; // Limit
    buy_order.tif = static_cast<uint8_t>(TimeInForce::GTC);

    size_t len1 = PacketEncoder::encode(buffer, sizeof(buffer), MessageType::NewOrder, 1, 1000, buy_order);
    transport.simulate_read(10, std::span<const uint8_t>(buffer, len1));

    // Give symbol threads some time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    transport.clear_sent();

    // 3. Submit Crossing Sell Order (10 shares @ 100.50)
    NewOrderPacket sell_order{};
    sell_order.client_order_id = 1002;
    sell_order.symbol_id = 1;
    sell_order.account_id = 98; // Different Account to cross
    sell_order.price_ticks = Price::from_double(100.50).ticks();
    sell_order.qty_units = Quantity::from_double(10.0).units();
    sell_order.side = 1; // Sell
    sell_order.type = 0; // Limit
    sell_order.tif = static_cast<uint8_t>(TimeInForce::GTC);

    size_t len2 = PacketEncoder::encode(buffer, sizeof(buffer), MessageType::NewOrder, 2, 1001, sell_order);
    
    // Register Sell Client
    (void)exchange->gateway().sessions().create_connection(20);
    (void)exchange->gateway().sessions().authenticate(20, 43, 98, "trader2", "pass");
    transport.simulate_read(20, std::span<const uint8_t>(buffer, len2));

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // 4. Verify trades/fills were generated and returned through the Gateway
    const auto& sent = transport.sent_messages();
    ASSERT_GT(sent.size(), 0);

    bool fill_found = false;
    for (const auto& msg : sent) {
        MessageHeader header;
        const uint8_t* payload = nullptr;
        auto bytes = PacketDecoder::decode(msg, header, payload);
        if (bytes.has_value() && header.type == static_cast<uint16_t>(MessageType::ExecutionReport)) {
            const auto* report = reinterpret_cast<const ExecutionReportPacket*>(payload - sizeof(MessageHeader));
            if (report->status == static_cast<uint8_t>(ExecutionType::Trade)) {
                fill_found = true;
                EXPECT_EQ(report->filled_qty, Quantity::from_double(10.0).units());
                EXPECT_EQ(report->price_ticks, Price::from_double(100.50).ticks());
            }
        }
    }
    EXPECT_TRUE(fill_found);
}

TEST_F(ExchangeTest, E2ERiskRejection) {
    ASSERT_TRUE(exchange->start("127.0.0.1", 12345));

    transport.simulate_connect(10);
    (void)exchange->gateway().sessions().authenticate(10, 42, 99, "trader1", "pass");

    // Submit Fat Finger Violation (100,000,000 units > max limit)
    uint8_t buffer[512];
    NewOrderPacket buy_order{};
    buy_order.client_order_id = 1001;
    buy_order.symbol_id = 1;
    buy_order.account_id = 99;
    buy_order.price_ticks = Price::from_double(100.50).ticks();
    buy_order.qty_units = Quantity::from_double(1000000.0).units(); // 1,000,000 shares (violates max limit!)
    buy_order.side = 0; // Buy

    size_t len = PacketEncoder::encode(buffer, sizeof(buffer), MessageType::NewOrder, 1, 1000, buy_order);
    transport.simulate_read(10, std::span<const uint8_t>(buffer, len));

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Verify rejection
    const auto& sent = transport.sent_messages();
    ASSERT_GT(sent.size(), 0);

    bool reject_found = false;
    for (const auto& msg : sent) {
        MessageHeader header;
        const uint8_t* payload = nullptr;
        auto bytes = PacketDecoder::decode(msg, header, payload);
        if (bytes.has_value() && header.type == static_cast<uint16_t>(MessageType::Reject)) {
            reject_found = true;
            const auto* reject = reinterpret_cast<const RejectPacket*>(payload - sizeof(MessageHeader));
            EXPECT_EQ(reject->client_order_id, 1001);
            EXPECT_EQ(reject->reason, static_cast<uint16_t>(RejectReason::InvalidQuantity));
        }
    }
    EXPECT_TRUE(reject_found);
}
