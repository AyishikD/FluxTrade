#pragma once

#include <cstdint>
#include <cstddef>
#include <span>

namespace fluxtrade {

class ITransportListener {
public:
    virtual ~ITransportListener() = default;
    virtual void on_connect(int32_t conn_id) = 0;
    virtual void on_disconnect(int32_t conn_id) = 0;
    virtual void on_read(int32_t conn_id, std::span<const uint8_t> data) = 0;
};

class ITransport {
public:
    virtual ~ITransport() = default;
    virtual bool start(const char* host, uint16_t port) = 0;
    virtual void stop() = 0;
    virtual void poll() = 0;
    virtual bool send(int32_t conn_id, std::span<const uint8_t> data) = 0;
    virtual void disconnect(int32_t conn_id) = 0;
    virtual void set_listener(ITransportListener* listener) = 0;
};

} // namespace fluxtrade
