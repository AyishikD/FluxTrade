#pragma once

#include <persistence/metadata_manager.hpp>
#include <persistence/snapshot_reader.hpp>
#include <persistence/disk_journal.hpp>
#include <matching/matching_engine.hpp>
#include <gateway/order_command.hpp>
#include <vector>
#include <string>
#include <filesystem>
#include <iostream>

namespace fluxtrade {
namespace persistence {

class ReplayEngine {
public:
    explicit ReplayEngine(const std::string& directory) : directory_(directory) {}

    bool recover(LimitOrderBook& book) {
        MetadataManager meta_mgr(directory_);
        auto meta_opt = meta_mgr.load();
        
        uint64_t last_seq = 0;
        uint64_t last_ts = 0;
        uint64_t start_journal = 1;
        
        if (meta_opt.has_value()) {
            std::string snapshot_file = directory_ + "/snapshot_" + std::to_string(meta_opt->latest_snapshot_sequence) + ".bin";
            if (std::filesystem::exists(snapshot_file)) {
                if (!SnapshotReader::load(snapshot_file, book, last_seq, last_ts)) {
                    std::cerr << "Failed to load snapshot: " << snapshot_file << std::endl;
                    return false;
                }
                start_journal = meta_opt->current_journal_index;
            }
        }
        
        // Find and replay journals
        for (uint64_t journal_idx = start_journal; ; ++journal_idx) {
            char filename[256];
            std::snprintf(filename, sizeof(filename), "%s/journal_%06llu.wal", directory_.c_str(), journal_idx);
            
            if (!std::filesystem::exists(filename)) {
                break; // No more journals
            }
            
            if (!replay_journal(filename, book, last_seq)) {
                std::cerr << "Error replaying journal: " << filename << std::endl;
                return false;
            }
        }
        
        return true;
    }

private:
    bool replay_journal(const std::string& filename, LimitOrderBook& book, uint64_t& current_seq) {
        std::ifstream file(filename, std::ios::in | std::ios::binary);
        if (!file.is_open()) return false;

        WALHeader header{};
        file.read(reinterpret_cast<char*>(&header), sizeof(WALHeader));
        if (file.gcount() != sizeof(WALHeader)) return false;
        
        if (header.magic != WAL_MAGIC || header.version != WAL_VERSION) return false;

        while (file.peek() != EOF) {
            WALRecordHeader rheader{};
            file.read(reinterpret_cast<char*>(&rheader), sizeof(WALRecordHeader));
            if (file.gcount() != sizeof(WALRecordHeader)) break;
            
            std::vector<std::byte> payload(rheader.payload_length);
            file.read(reinterpret_cast<char*>(payload.data()), rheader.payload_length);
            if (file.gcount() != rheader.payload_length) break;
            
            // Checksum validation
            uint32_t expected = rheader.checksum;
            uint32_t crc = Checksum::crc32c(&rheader, offsetof(WALRecordHeader, checksum));
            crc = Checksum::crc32c(payload.data(), payload.size(), ~crc);
            if (expected != crc) {
                std::cerr << "Checksum mismatch on seq " << rheader.sequence << std::endl;
                return false;
            }
            
            if (rheader.sequence <= current_seq) continue; // Already applied in snapshot
            
            // Apply payload
            if (rheader.message_type == static_cast<uint16_t>(CommandType::NewOrder)) {
                if (payload.size() == sizeof(OrderCommand)) {
                    const OrderCommand* cmd = reinterpret_cast<const OrderCommand*>(payload.data());
                    
                    Order order{
                        .order_id = OrderId(cmd->client_order_id),
                        .price = cmd->price,
                        .qty = cmd->qty,
                        .seq_num = cmd->sequence,
                        .client_timestamp = cmd->timestamp,
                        .exchange_timestamp = rheader.timestamp,
                        .client_id = ClientId(1),
                        .account_id = cmd->account_id,
                        .symbol_id = cmd->symbol_id,
                        .side = cmd->side,
                        .type = cmd->order_type,
                        .tif = cmd->tif,
                        .flags = OrderFlags::None
                    };
                    
                    book.insert(order, cmd->sequence);
                }
            } else if (rheader.message_type == static_cast<uint16_t>(CommandType::CancelOrder)) {
                 if (payload.size() == sizeof(OrderCommand)) {
                    const OrderCommand* cmd = reinterpret_cast<const OrderCommand*>(payload.data());
                    book.cancel(OrderId(cmd->orig_client_order_id));
                 }
            }
            
            current_seq = rheader.sequence;
        }
        return true;
    }
};

} // namespace persistence
} // namespace fluxtrade
