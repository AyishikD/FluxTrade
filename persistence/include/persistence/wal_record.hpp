#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>

namespace fluxtrade {
namespace persistence {

// Magic bytes: 'F' 'L' 'U' 'X'
constexpr uint32_t WAL_MAGIC = 0x58554C46;
constexpr uint16_t WAL_VERSION = 1;

struct alignas(8) WALHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint64_t start_sequence;
    uint64_t timestamp;
    uint8_t padding[8] = {0};
};
static_assert(sizeof(WALHeader) == 32, "WALHeader must be exactly 32 bytes");
static_assert(std::is_trivially_copyable_v<WALHeader>, "WALHeader must be trivially copyable");

struct alignas(8) WALRecordHeader {
    uint64_t sequence;
    uint64_t timestamp;
    uint16_t message_type;
    uint16_t payload_length;
    uint32_t checksum;
};
static_assert(sizeof(WALRecordHeader) == 24, "WALRecordHeader must be exactly 24 bytes");
static_assert(std::is_trivially_copyable_v<WALRecordHeader>, "WALRecordHeader must be trivially copyable");

// A wrapper to point to the header and the variable-length payload memory
struct WALRecordView {
    const WALRecordHeader* header;
    const std::byte* payload;

    [[nodiscard]] size_t total_size() const noexcept {
        return sizeof(WALRecordHeader) + (header ? header->payload_length : 0);
    }
};

} // namespace persistence
} // namespace fluxtrade
