#pragma once

#include <gateway/protocol.hpp>
#include <cstddef>
#include <span>
#include <optional>

namespace fluxtrade {

class PacketDecoder {
public:
    [[nodiscard]] static std::optional<size_t> decode(
        std::span<const uint8_t> data,
        MessageHeader& out_header,
        const uint8_t*& out_payload
    ) noexcept {
        if (data.size() < sizeof(MessageHeader)) {
            return std::nullopt;
        }
        
        const auto* header = reinterpret_cast<const MessageHeader*>(data.data());
        if (data.size() < header->length) {
            return std::nullopt;
        }
        
        out_header = *header;
        out_payload = data.data() + sizeof(MessageHeader);
        return header->length;
    }
};

} // namespace fluxtrade
