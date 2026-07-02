#pragma once

#include <persistence/memory_journal.hpp>
#include <persistence/wal_record.hpp>
#include <common/ring_buffer.hpp>
#include <common/thread_pinning.hpp>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <cstdio>

namespace fluxtrade {
namespace persistence {

enum class FlushPolicy {
    Never,
    Interval,
    Batch,
    Immediate
};

class DiskJournal {
public:
    DiskJournal(const std::string& directory, FlushPolicy policy, size_t max_file_size = 512 * 1024 * 1024)
        : directory_(directory), policy_(policy), max_file_size_(max_file_size) {
        std::filesystem::create_directories(directory_);
    }

    ~DiskJournal() { stop(); }

    DiskJournal(const DiskJournal&) = delete;
    DiskJournal& operator=(const DiskJournal&) = delete;

    void start(RingBuffer<LogEntry, 65536>& queue, uint32_t core_id = 0) {
        if (running_.load(std::memory_order_relaxed)) return;
        running_.store(true, std::memory_order_relaxed);
        thread_ = std::thread([this, &queue, core_id]() { run(queue, core_id); });
    }

    void stop() {
        if (!running_.load(std::memory_order_relaxed)) return;
        running_.store(false, std::memory_order_relaxed);
        if (thread_.joinable()) thread_.join();
        if (current_file_.is_open()) {
            current_file_.flush();
            current_file_.close();
        }
    }

private:
    void run(RingBuffer<LogEntry, 65536>& queue, uint32_t core_id) {
        if (core_id > 0) pin_thread(core_id);

        LogEntry entry;
        size_t batch_count = 0;
        auto last_flush = std::chrono::steady_clock::now();

        while (running_.load(std::memory_order_relaxed) || !queue.empty()) {
            if (queue.pop(entry)) {
                write_entry(entry);
                batch_count++;

                if (policy_ == FlushPolicy::Immediate) {
                    flush();
                } else if (policy_ == FlushPolicy::Batch && batch_count >= 1000) {
                    flush();
                    batch_count = 0;
                }
            } else {
                if (policy_ == FlushPolicy::Interval) {
                    auto now = std::chrono::steady_clock::now();
                    if (now - last_flush >= std::chrono::milliseconds(10)) {
                        flush();
                        last_flush = now;
                    }
                }
                std::this_thread::yield();
            }
        }
        flush();
    }

    void write_entry(const LogEntry& entry) {
        if (!current_file_.is_open() || current_bytes_ >= max_file_size_) {
            rotate_file(entry.header.sequence);
        }

        current_file_.write(reinterpret_cast<const char*>(&entry.header), sizeof(WALRecordHeader));
        current_file_.write(reinterpret_cast<const char*>(entry.payload.data()), entry.header.payload_length);
        current_bytes_ += sizeof(WALRecordHeader) + entry.header.payload_length;
    }

    void flush() {
        if (current_file_.is_open() && policy_ != FlushPolicy::Never) {
            current_file_.flush();
        }
    }

    void rotate_file(uint64_t sequence) {
        if (current_file_.is_open()) {
            current_file_.flush();
            current_file_.close();
        }

        file_index_++;
        char filename[256];
        std::snprintf(filename, sizeof(filename), "%s/journal_%06zu.wal", directory_.c_str(), file_index_);
        
        current_file_.open(filename, std::ios::out | std::ios::binary | std::ios::trunc);
        
        WALHeader header{};
        header.magic = WAL_MAGIC;
        header.version = WAL_VERSION;
        header.flags = 0;
        header.start_sequence = sequence;
        header.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        
        current_file_.write(reinterpret_cast<const char*>(&header), sizeof(WALHeader));
        current_bytes_ = sizeof(WALHeader);
    }

    std::string directory_;
    FlushPolicy policy_;
    size_t max_file_size_;
    std::atomic<bool> running_{false};
    std::thread thread_;
    std::ofstream current_file_;
    size_t current_bytes_ = 0;
    size_t file_index_ = 0;
};

} // namespace persistence
} // namespace fluxtrade
