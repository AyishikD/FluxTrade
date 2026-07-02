#pragma once

#include <persistence/wal_record.hpp>
#include <persistence/checksum.hpp>
#include <common/ring_buffer.hpp>
#include <array>
#include <cstring>

namespace fluxtrade {
namespace persistence {

// Fixed-size envelope for zero-allocation queue passing
struct alignas(8) LogEntry {
    WALRecordHeader header;
    std::array<std::byte, 256> payload;
};
static_assert(sizeof(LogEntry) == 280, "LogEntry must be 280 bytes");

class MemoryJournal {
public:
    MemoryJournal() = default;
    ~MemoryJournal() = default;

    MemoryJournal(const MemoryJournal&) = delete;
    MemoryJournal& operator=(const MemoryJournal&) = delete;

    template<size_t Capacity>
    [[nodiscard]] static bool append(
        RingBuffer<LogEntry, Capacity>& queue, 
        uint64_t sequence, 
        uint64_t timestamp, 
        uint16_t msg_type, 
        const void* data, 
        size_t length
    ) noexcept {
        if (length > 256) return false;

        LogEntry entry;
        entry.header.sequence = sequence;
        entry.header.timestamp = timestamp;
        entry.header.message_type = msg_type;
        entry.header.payload_length = static_cast<uint16_t>(length);
        
        // Copy payload
        std::memcpy(entry.payload.data(), data, length);

        uint32_t crc = Checksum::crc32c(&entry.header, offsetof(WALRecordHeader, checksum));
        crc = Checksum::crc32c(data, length, ~crc);
        entry.header.checksum = crc;

        return queue.push(entry);
    }
};

} // namespace persistence
} // namespace fluxtrade
