#pragma once

#include <gateway/transport.hpp>
#include <vector>

namespace fluxtrade {

class SimulationTransport : public ITransport {
public:
    SimulationTransport() = default;
    virtual ~SimulationTransport() = default;

    bool start(const char*, uint16_t) override { return true; }
    void stop() override {}
    void poll() override {}
    
    bool send(int32_t, std::span<const uint8_t> data) override {
        sent_messages_.push_back(std::vector<uint8_t>(data.begin(), data.end()));
        return true;
    }
    
    void disconnect(int32_t conn_id) override {
        simulate_disconnect(conn_id);
    }

    void set_listener(ITransportListener* listener) override {
        listener_ = listener;
    }

    void simulate_connect(int32_t conn_id) {
        if (listener_) listener_->on_connect(conn_id);
    }

    void simulate_disconnect(int32_t conn_id) {
        if (listener_) listener_->on_disconnect(conn_id);
    }

    void simulate_read(int32_t conn_id, std::span<const uint8_t> data) {
        if (listener_) listener_->on_read(conn_id, data);
    }

    [[nodiscard]] const std::vector<std::vector<uint8_t>>& sent_messages() const { return sent_messages_; }
    void clear_sent() { sent_messages_.clear(); }

private:
    ITransportListener* listener_ = nullptr;
    std::vector<std::vector<uint8_t>> sent_messages_;
};

} // namespace fluxtrade
