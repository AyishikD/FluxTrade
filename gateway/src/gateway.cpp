#include <gateway/gateway.hpp>
#include <gateway/packet_decoder.hpp>
#include <gateway/packet_encoder.hpp>
#include <kernel/clock.hpp>
#include <domain/reject_reason.hpp>
#include <cstring>

namespace fluxtrade {

Gateway::Gateway(ITransport* transport, RingBuffer<OrderCommand, 4096>* inbound_queue) noexcept
    : transport_(transport), inbound_queue_(inbound_queue) {
    if (transport_) {
        transport_->set_listener(this);
    }
}

Gateway::~Gateway() {
    stop();
}

bool Gateway::start(const char* host, uint16_t port) noexcept {
    if (!transport_) return false;
    return transport_->start(host, port);
}

void Gateway::stop() noexcept {
    if (transport_) {
        transport_->stop();
    }
    session_manager_.reset();
}

void Gateway::poll() noexcept {
    if (transport_) {
        transport_->poll();
    }
}

void Gateway::on_connect(int32_t conn_id) {
    (void)session_manager_.create_connection(conn_id);
}

void Gateway::on_disconnect(int32_t conn_id) {
    session_manager_.remove_connection(conn_id);
}

void Gateway::on_read(int32_t conn_id, std::span<const uint8_t> data) {
    ClientConnection* conn = session_manager_.get_connection(conn_id);
    if (!conn) return;

    // Check if new data overflows internal buffer
    if (conn->buffer_len + data.size() > ClientConnection::BUFFER_SIZE) {
        // Force disconnect client due to overflow
        transport_->disconnect(conn_id);
        return;
    }

    std::memcpy(conn->buffer + conn->buffer_len, data.data(), data.size());
    conn->buffer_len += data.size();

    // Frame parser loop
    size_t processed = 0;
    while (processed < conn->buffer_len) {
        std::span<const uint8_t> frame(conn->buffer + processed, conn->buffer_len - processed);
        MessageHeader header;
        const uint8_t* payload = nullptr;
        auto bytes_read = PacketDecoder::decode(frame, header, payload);
        if (!bytes_read.has_value()) {
            break; // Incomplete frame, wait for more packets
        }
        
        handle_packet(conn_id, header, payload);
        processed += bytes_read.value();
    }

    // Shift residual bytes in framing buffer
    if (processed > 0) {
        if (processed < conn->buffer_len) {
            std::memmove(conn->buffer, conn->buffer + processed, conn->buffer_len - processed);
            conn->buffer_len -= processed;
        } else {
            conn->buffer_len = 0;
        }
    }
}

template <typename PacketType>
bool Gateway::send_packet(
    int32_t conn_id,
    MessageType type,
    uint64_t seq,
    uint64_t ts,
    const PacketType& packet
) noexcept {
    if (!transport_) return false;
    size_t len = PacketEncoder::encode(write_buffer_, sizeof(write_buffer_), type, seq, ts, packet);
    if (len == 0) return false;
    return transport_->send(conn_id, std::span<const uint8_t>(write_buffer_, len));
}

// Explicit instantiations of response packets
template bool Gateway::send_packet<LoginResponsePacket>(int32_t, MessageType, uint64_t, uint64_t, const LoginResponsePacket&) noexcept;
template bool Gateway::send_packet<RejectPacket>(int32_t, MessageType, uint64_t, uint64_t, const RejectPacket&) noexcept;
template bool Gateway::send_packet<ExecutionReportPacket>(int32_t, MessageType, uint64_t, uint64_t, const ExecutionReportPacket&) noexcept;
template bool Gateway::send_packet<HeartbeatPacket>(int32_t, MessageType, uint64_t, uint64_t, const HeartbeatPacket&) noexcept;

void Gateway::handle_packet(int32_t conn_id, const MessageHeader& header, const uint8_t* payload) noexcept {
    ClientConnection* conn = session_manager_.get_connection(conn_id);
    if (!conn) return;

    const MessageType type = static_cast<MessageType>(header.type);

    if (type == MessageType::LoginRequest) {
        const auto* packet = reinterpret_cast<const LoginRequestPacket*>(payload - sizeof(MessageHeader));
        bool ok = session_manager_.authenticate(conn_id, packet->client_id, packet->account_id, packet->username, packet->password);
        
        LoginResponsePacket resp{};
        resp.client_id = packet->client_id;
        resp.account_id = packet->account_id;
        resp.success = ok ? 1 : 0;
        if (!ok) {
            std::strncpy(resp.reject_reason, "Auth Failure", sizeof(resp.reject_reason));
        }

        send_packet(conn_id, MessageType::LoginResponse, 0, Clock::now_steady(), resp);
        return;
    }

    // Reject unauthenticated requests
    if (!conn->authenticated) {
        RejectPacket reject{};
        reject.client_order_id = 0;
        reject.account_id = 0;
        reject.reason = static_cast<uint16_t>(RejectReason::None); // Unauthenticated reject
        send_packet(conn_id, MessageType::Reject, 0, Clock::now_steady(), reject);
        return;
    }

    if (type == MessageType::Heartbeat) {
        conn->last_heartbeat_ts = Clock::now_steady();
        HeartbeatPacket echo{};
        send_packet(conn_id, MessageType::Heartbeat, 0, Clock::now_steady(), echo);
        return;
    }

    if (type == MessageType::Logout) {
        conn->authenticated = false;
        return;
    }

    // Process order commands
    if (type == MessageType::NewOrder) {
        const auto* packet = reinterpret_cast<const NewOrderPacket*>(payload - sizeof(MessageHeader));
        
        OrderCommand cmd;
        cmd.type = CommandType::NewOrder;
        cmd.sequence = sequence_manager_.next_sequence();
        cmd.timestamp = Clock::now_steady();
        cmd.client_order_id = packet->client_order_id;
        cmd.symbol_id = SymbolId(packet->symbol_id);
        cmd.account_id = AccountId(packet->account_id);
        cmd.price = Price(packet->price_ticks);
        cmd.qty = Quantity(packet->qty_units);
        cmd.side = packet->side == 0 ? Side::Buy : Side::Sell;
        cmd.order_type = packet->type == 0 ? OrderType::Limit : OrderType::Market;
        cmd.tif = static_cast<TimeInForce>(packet->tif);
        cmd.conn_id = conn_id;

        inbound_queue_->push(cmd);
        return;
    }

    if (type == MessageType::CancelOrder) {
        const auto* packet = reinterpret_cast<const CancelOrderPacket*>(payload - sizeof(MessageHeader));
        
        OrderCommand cmd;
        cmd.type = CommandType::CancelOrder;
        cmd.sequence = sequence_manager_.next_sequence();
        cmd.timestamp = Clock::now_steady();
        cmd.client_order_id = packet->client_order_id;
        cmd.orig_client_order_id = packet->orig_client_order_id;
        cmd.symbol_id = SymbolId(packet->symbol_id);
        cmd.account_id = AccountId(packet->account_id);
        cmd.conn_id = conn_id;

        inbound_queue_->push(cmd);
        return;
    }

    if (type == MessageType::ReplaceOrder) {
        const auto* packet = reinterpret_cast<const ReplaceOrderPacket*>(payload - sizeof(MessageHeader));
        
        OrderCommand cmd;
        cmd.type = CommandType::ReplaceOrder;
        cmd.sequence = sequence_manager_.next_sequence();
        cmd.timestamp = Clock::now_steady();
        cmd.client_order_id = packet->client_order_id;
        cmd.orig_client_order_id = packet->orig_client_order_id;
        cmd.symbol_id = SymbolId(packet->symbol_id);
        cmd.account_id = AccountId(packet->account_id);
        cmd.price = Price(packet->price_ticks);
        cmd.qty = Quantity(packet->qty_units);
        cmd.conn_id = conn_id;

        inbound_queue_->push(cmd);
        return;
    }
}

} // namespace fluxtrade
