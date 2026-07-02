#pragma once

#include <matching/order_book/order_book.hpp>
#include <persistence/snapshot_writer.hpp>
#include <persistence/checksum.hpp>
#include <fstream>
#include <string>
#include <optional>

namespace fluxtrade {
namespace persistence {

class SnapshotReader {
public:
    static bool load(const std::string& filepath, LimitOrderBook& book, uint64_t& out_sequence, uint64_t& out_timestamp) {
        std::ifstream file(filepath, std::ios::in | std::ios::binary);
        if (!file.is_open()) return false;

        SnapshotHeader header{};
        file.read(reinterpret_cast<char*>(&header), sizeof(SnapshotHeader));
        if (file.gcount() != sizeof(SnapshotHeader)) return false;
        
        if (header.magic != SNAPSHOT_MAGIC || header.version != SNAPSHOT_VERSION) return false;
        
        out_sequence = header.sequence;
        out_timestamp = header.timestamp;
        
        uint32_t expected_crc = header.checksum;
        uint32_t crc = 0xFFFFFFFF;

        book.clear();

        // Read bids
        if (!read_levels(file, book, header.bid_level_count, Side::Buy, crc)) return false;
        // Read asks
        if (!read_levels(file, book, header.ask_level_count, Side::Sell, crc)) return false;
        
        if (expected_crc != ~crc) return false;

        book.verify_invariants();
        return true;
    }

private:
    static bool read_levels(std::ifstream& file, LimitOrderBook& book, uint32_t level_count, Side side, uint32_t& crc) {
        for (uint32_t i = 0; i < level_count; ++i) {
            LevelSnapshot lsnap{};
            file.read(reinterpret_cast<char*>(&lsnap), sizeof(LevelSnapshot));
            if (file.gcount() != sizeof(LevelSnapshot)) return false;
            
            crc = Checksum::crc32c(&lsnap, sizeof(LevelSnapshot), ~crc);
            
            for (uint32_t j = 0; j < lsnap.order_count; ++j) {
                OrderSnapshot osnap{};
                file.read(reinterpret_cast<char*>(&osnap), sizeof(OrderSnapshot));
                if (file.gcount() != sizeof(OrderSnapshot)) return false;
                
                crc = Checksum::crc32c(&osnap, sizeof(OrderSnapshot), ~crc);
                
                // Reconstruct a dummy Order to insert into the book
                Order dummy{};
                dummy.order_id = osnap.key.order_id;
                dummy.symbol_id = osnap.key.symbol_id;
                dummy.price = lsnap.price;
                dummy.qty = osnap.remaining_qty;
                dummy.side = side;
                dummy.type = OrderType::Limit;
                dummy.tif = TimeInForce::GTC;
                dummy.client_id = ClientId(0);
                dummy.account_id = AccountId(0);
                
                // Insert restores the pointers perfectly using standard operations!
                book.insert(dummy, osnap.priority_seq);
            }
        }
        return true;
    }
};

} // namespace persistence
} // namespace fluxtrade
