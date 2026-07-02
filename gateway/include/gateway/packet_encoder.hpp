#pragma once

#include <gateway/protocol.hpp>
#include <cstddef>

namespace fluxtrade {

class PacketEncoder {
public:
    template <typename PacketType>
    [[nodiscard]] static size_t encode(
        uint8_t* dest,
        size_t dest_len,
        MessageType type,
        uint64_t seq,
        uint64_t ts,
        const PacketType& packet
    ) noexcept {
        if (dest_len < sizeof(PacketType)) {
            return 0;
        }
        
        auto* out = reinterpret_cast<PacketType*>(dest);
        *out = packet;
        out->header.length = static_cast<uint16_t>(sizeof(PacketType));
        out->header.type = static_cast<uint16_t>(type);
        out->header.sequence = seq;
        out->header.timestamp = ts;
        return sizeof(PacketType);
    }
};

} // namespace fluxtrade
