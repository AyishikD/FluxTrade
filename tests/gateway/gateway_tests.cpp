#include <gtest/gtest.h>
#include <gateway/gateway.hpp>
#include <gateway/simulation_transport.hpp>
#include <gateway/packet_encoder.hpp>
#include <gateway/packet_decoder.hpp>
#include <cstring>

using namespace fluxtrade;

class GatewayTest : public ::testing::Test {
protected:
    void SetUp() override {
        gateway = std::make_unique<Gateway>(&transport, &inbound_queue);
    }

    SimulationTransport transport;
    RingBuffer<OrderCommand, 4096> inbound_queue;
    std::unique_ptr<Gateway> gateway;
};

TEST_F(GatewayTest, PacketEncodeDecodeRoundtrip) {
    uint8_t buffer[512];
    LoginRequestPacket req{};
    req.client_id = 42;
    req.account_id = 99;
    std::strncpy(req.username, "trader1", sizeof(req.username));
    std::strncpy(req.password, "pass123", sizeof(req.password));

    size_t len = PacketEncoder::encode(buffer, sizeof(buffer), MessageType::LoginRequest, 101, 202, req);
    ASSERT_GT(len, 0);
    EXPECT_EQ(len, sizeof(LoginRequestPacket));

    MessageHeader header;
    const uint8_t* payload = nullptr;
    auto bytes_read = PacketDecoder::decode(std::span<const uint8_t>(buffer, len), header, payload);

    ASSERT_TRUE(bytes_read.has_value());
    EXPECT_EQ(bytes_read.value(), len);
    EXPECT_EQ(header.type, static_cast<uint16_t>(MessageType::LoginRequest));
    EXPECT_EQ(header.sequence, 101);
    EXPECT_EQ(header.timestamp, 202);

    const auto* decoded_req = reinterpret_cast<const LoginRequestPacket*>(payload - sizeof(MessageHeader));
    EXPECT_EQ(decoded_req->client_id, 42);
    EXPECT_EQ(decoded_req->account_id, 99);
    EXPECT_STREQ(decoded_req->username, "trader1");
}

TEST_F(GatewayTest, SessionAuthenticationSuccess) {
    transport.simulate_connect(10);
    
    // Send Login request
    uint8_t buffer[512];
    LoginRequestPacket req{};
    req.client_id = 42;
    req.account_id = 99;
    std::strncpy(req.username, "trader1", sizeof(req.username));
    std::strncpy(req.password, "pass123", sizeof(req.password));

    size_t len = PacketEncoder::encode(buffer, sizeof(buffer), MessageType::LoginRequest, 1, 1000, req);
    transport.simulate_read(10, std::span<const uint8_t>(buffer, len));
    gateway->poll();

    // Verify response
    const auto& sent = transport.sent_messages();
    ASSERT_EQ(sent.size(), 1);
    
    MessageHeader header;
    const uint8_t* payload = nullptr;
    auto bytes = PacketDecoder::decode(sent[0], header, payload);
    ASSERT_TRUE(bytes.has_value());
    EXPECT_EQ(header.type, static_cast<uint16_t>(MessageType::LoginResponse));

    const auto* resp = reinterpret_cast<const LoginResponsePacket*>(payload - sizeof(MessageHeader));
    EXPECT_EQ(resp->success, 1);
    EXPECT_EQ(resp->client_id, 42);

    // Verify connection state
    ClientConnection* conn = gateway->sessions().get_connection(10);
    ASSERT_NE(conn, nullptr);
    EXPECT_TRUE(conn->authenticated);
}

TEST_F(GatewayTest, SessionAuthenticationFailure) {
    transport.simulate_connect(10);
    
    // Send Login request with empty username
    uint8_t buffer[512];
    LoginRequestPacket req{};
    req.client_id = 42;
    req.account_id = 99;
    req.username[0] = '\0';
    std::strncpy(req.password, "pass123", sizeof(req.password));

    size_t len = PacketEncoder::encode(buffer, sizeof(buffer), MessageType::LoginRequest, 1, 1000, req);
    transport.simulate_read(10, std::span<const uint8_t>(buffer, len));
    gateway->poll();

    const auto& sent = transport.sent_messages();
    ASSERT_EQ(sent.size(), 1);
    
    MessageHeader header;
    const uint8_t* payload = nullptr;
    auto bytes = PacketDecoder::decode(sent[0], header, payload);
    ASSERT_TRUE(bytes.has_value());

    const auto* resp = reinterpret_cast<const LoginResponsePacket*>(payload - sizeof(MessageHeader));
    EXPECT_EQ(resp->success, 0);
    EXPECT_STREQ(resp->reject_reason, "Auth Failure");
}

TEST_F(GatewayTest, RejectUnauthenticatedCommands) {
    transport.simulate_connect(10);

    // Send New Order packet without logging in
    uint8_t buffer[512];
    NewOrderPacket ord{};
    ord.client_order_id = 1001;
    ord.symbol_id = 1;
    ord.account_id = 99;

    size_t len = PacketEncoder::encode(buffer, sizeof(buffer), MessageType::NewOrder, 1, 1000, ord);
    transport.simulate_read(10, std::span<const uint8_t>(buffer, len));
    gateway->poll();

    // Verify we receive a Reject packet
    const auto& sent = transport.sent_messages();
    ASSERT_EQ(sent.size(), 1);
    
    MessageHeader header;
    const uint8_t* payload = nullptr;
    auto bytes = PacketDecoder::decode(sent[0], header, payload);
    ASSERT_TRUE(bytes.has_value());
    EXPECT_EQ(header.type, static_cast<uint16_t>(MessageType::Reject));
}

TEST_F(GatewayTest, HeartbeatEcho) {
    transport.simulate_connect(10);
    
    // Login
    (void)gateway->sessions().authenticate(10, 42, 99, "trader1", "pass");

    // Send Heartbeat
    uint8_t buffer[512];
    HeartbeatPacket hb{};
    size_t len = PacketEncoder::encode(buffer, sizeof(buffer), MessageType::Heartbeat, 1, 1000, hb);
    transport.simulate_read(10, std::span<const uint8_t>(buffer, len));
    gateway->poll();

    // Verify echo
    const auto& sent = transport.sent_messages();
    ASSERT_EQ(sent.size(), 1);

    MessageHeader header;
    const uint8_t* payload = nullptr;
    auto bytes = PacketDecoder::decode(sent[0], header, payload);
    ASSERT_TRUE(bytes.has_value());
    EXPECT_EQ(header.type, static_cast<uint16_t>(MessageType::Heartbeat));
}
