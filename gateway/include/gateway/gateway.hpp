#pragma once

#include <gateway/transport.hpp>
#include <gateway/session_manager.hpp>
#include <gateway/sequence_manager.hpp>
#include <gateway/order_command.hpp>
#include <common/ring_buffer.hpp>
#include <span>
#include <optional>

namespace fluxtrade {

class Gateway : public ITransportListener {
public:
    Gateway(ITransport* transport, RingBuffer<OrderCommand, 4096>* inbound_queue) noexcept;
    virtual ~Gateway();

    Gateway(const Gateway&) = delete;
    Gateway& operator=(const Gateway&) = delete;

    bool start(const char* host, uint16_t port) noexcept;
    void stop() noexcept;
    void poll() noexcept;

    // Outbound response formatting helper
    template <typename PacketType>
    bool send_packet(int32_t conn_id, MessageType type, uint64_t seq, uint64_t ts, const PacketType& packet) noexcept;

    // ITransportListener callbacks
    void on_connect(int32_t conn_id) override;
    void on_disconnect(int32_t conn_id) override;
    void on_read(int32_t conn_id, std::span<const uint8_t> data) override;

    [[nodiscard]] SessionManager& sessions() noexcept { return session_manager_; }
    [[nodiscard]] SequenceManager& sequences() noexcept { return sequence_manager_; }

private:
    void handle_packet(int32_t conn_id, const MessageHeader& header, const uint8_t* payload) noexcept;

    ITransport* transport_;
    RingBuffer<OrderCommand, 4096>* inbound_queue_;
    SessionManager session_manager_;
    SequenceManager sequence_manager_;
    uint8_t write_buffer_[8192];
};

} // namespace fluxtrade
