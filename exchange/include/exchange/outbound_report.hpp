#pragma once

#include <gateway/protocol.hpp>
#include <cstdint>
#include <cstring>

namespace fluxtrade {

struct alignas(8) OutboundReport {
    enum class Type : uint8_t {
        ExecutionReport = 0,
        Reject          = 1
    };

    Type type = Type::Reject;
    uint8_t padding[3] = {0, 0, 0};
    int32_t conn_id = -1;
    
    union Data {
        ExecutionReportPacket execution;
        RejectPacket reject;

        Data() noexcept { std::memset(static_cast<void*>(this), 0, sizeof(*this)); }
        ~Data() noexcept {}
    } data;

    OutboundReport() noexcept : type(Type::Reject), conn_id(-1) {}
};

} // namespace fluxtrade
