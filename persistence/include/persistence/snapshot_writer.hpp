#pragma once

#include <matching/order_book/order_book.hpp>
#include <persistence/checksum.hpp>
#include <fstream>
#include <string>
#include <cstdint>

namespace fluxtrade {
namespace persistence {

constexpr uint32_t SNAPSHOT_MAGIC = 0x534E4150; // 'S' 'N' 'A' 'P'
constexpr uint16_t SNAPSHOT_VERSION = 1;

struct alignas(8) SnapshotHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint64_t sequence;
    uint64_t timestamp;
    uint32_t bid_level_count;
    uint32_t ask_level_count;
    uint32_t checksum;
    uint32_t padding;
};

struct LevelSnapshot {
    Price price;
    Quantity total_qty;
    uint32_t order_count;
    uint32_t padding;
};

struct OrderSnapshot {
    OrderKey key;
    Quantity remaining_qty;
    uint64_t priority_seq;
};

class SnapshotWriter {
public:
    static bool save(const std::string& filepath, const LimitOrderBook& book, uint64_t sequence, uint64_t timestamp) {
        std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!file.is_open()) return false;

        SnapshotHeader header{};
        header.magic = SNAPSHOT_MAGIC;
        header.version = SNAPSHOT_VERSION;
        header.sequence = sequence;
        header.timestamp = timestamp;
        
        // Count levels
        header.bid_level_count = count_levels(book.best_bid_level());
        header.ask_level_count = count_levels(book.best_ask_level());
        
        // Write placeholder header (will overwrite checksum later)
        auto header_pos = file.tellp();
        file.write(reinterpret_cast<const char*>(&header), sizeof(SnapshotHeader));
        
        uint32_t crc = 0xFFFFFFFF;

        // Write bids
        crc = write_levels(file, book.best_bid_level(), crc);
        // Write asks
        crc = write_levels(file, book.best_ask_level(), crc);
        
        header.checksum = ~crc; // We finalize checksum here, not chaining it further.
        
        // Overwrite header with checksum
        file.seekp(header_pos);
        file.write(reinterpret_cast<const char*>(&header), sizeof(SnapshotHeader));
        
        file.flush();
        return true;
    }

private:
    static uint32_t count_levels(PriceLevelNode* start) {
        uint32_t count = 0;
        PriceLevelNode* curr = start;
        while (curr) {
            count++;
            curr = curr->next;
        }
        return count;
    }

    static uint32_t write_levels(std::ofstream& file, PriceLevelNode* start, uint32_t initial_crc) {
        uint32_t crc = initial_crc;
        PriceLevelNode* curr = start;
        
        while (curr) {
            LevelSnapshot lsnap{};
            lsnap.price = curr->price;
            lsnap.total_qty = curr->total_qty;
            lsnap.order_count = count_orders(curr->head_order);
            
            file.write(reinterpret_cast<const char*>(&lsnap), sizeof(LevelSnapshot));
            crc = Checksum::crc32c(&lsnap, sizeof(LevelSnapshot), ~crc);
            
            LiveOrder* order = curr->head_order;
            while (order) {
                OrderSnapshot osnap{};
                osnap.key = order->key;
                osnap.remaining_qty = order->remaining_qty;
                osnap.priority_seq = order->priority_seq;
                
                file.write(reinterpret_cast<const char*>(&osnap), sizeof(OrderSnapshot));
                crc = Checksum::crc32c(&osnap, sizeof(OrderSnapshot), ~crc);
                
                order = order->next;
            }
            
            curr = curr->next;
        }
        return crc;
    }
    
    static uint32_t count_orders(LiveOrder* start) {
        uint32_t count = 0;
        LiveOrder* curr = start;
        while (curr) {
            count++;
            curr = curr->next;
        }
        return count;
    }
};

} // namespace persistence
} // namespace fluxtrade
