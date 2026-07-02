#include <gtest/gtest.h>
#include <persistence/wal_record.hpp>
#include <persistence/checksum.hpp>
#include <persistence/memory_journal.hpp>
#include <persistence/disk_journal.hpp>
#include <common/ring_buffer.hpp>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

using namespace fluxtrade;
using namespace fluxtrade::persistence;

TEST(WalTests, ChecksumValidation) {
    const char* data = "hello world";
    uint32_t crc1 = Checksum::crc32c(data, 11);
    EXPECT_NE(crc1, 0);
    
    // Check chaining
    uint32_t crc_chain = Checksum::crc32c("hello ", 6);
    crc_chain = Checksum::crc32c("world", 5, ~crc_chain);
    EXPECT_EQ(crc1, crc_chain);
}

TEST(WalTests, MemoryJournalAppend) {
    RingBuffer<LogEntry, 1024> queue;
    const char* payload = "test payload";
    
    bool appended = MemoryJournal::append(
        queue, 1, 1000, 2, payload, 12
    );
    
    EXPECT_TRUE(appended);
    
    LogEntry entry;
    EXPECT_TRUE(queue.pop(entry));
    EXPECT_EQ(entry.header.sequence, 1);
    EXPECT_EQ(entry.header.message_type, 2);
    EXPECT_EQ(entry.header.payload_length, 12);
    
    // Checksum verification
    uint32_t crc = Checksum::crc32c(&entry.header, offsetof(WALRecordHeader, checksum));
    crc = Checksum::crc32c(entry.payload.data(), 12, ~crc);
    EXPECT_EQ(entry.header.checksum, crc);
}

TEST(WalTests, DiskJournalIntegration) {
    std::string test_dir = "test_wal_dir";
    std::filesystem::remove_all(test_dir);
    
    RingBuffer<LogEntry, 1024> queue;
    DiskJournal journal(test_dir, FlushPolicy::Immediate, 1024 * 1024);
    
    journal.start(queue);
    
    const char* payload1 = "msg1";
    MemoryJournal::append(queue, 1, 100, 1, payload1, 4);
    
    const char* payload2 = "msg2";
    MemoryJournal::append(queue, 2, 200, 1, payload2, 4);
    
    // Wait for async flush
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    journal.stop();
    
    // Verify file created
    bool found = false;
    for (const auto& entry : std::filesystem::directory_iterator(test_dir)) {
        if (entry.path().extension() == ".wal") {
            found = true;
            std::ifstream file(entry.path(), std::ios::binary);
            EXPECT_TRUE(file.is_open());
            
            WALHeader header;
            file.read(reinterpret_cast<char*>(&header), sizeof(WALHeader));
            EXPECT_EQ(header.magic, WAL_MAGIC);
            
            WALRecordHeader rheader;
            file.read(reinterpret_cast<char*>(&rheader), sizeof(WALRecordHeader));
            EXPECT_EQ(rheader.sequence, 1);
            EXPECT_EQ(rheader.payload_length, 4);
            
            break;
        }
    }
    EXPECT_TRUE(found);
    
    std::filesystem::remove_all(test_dir);
}
