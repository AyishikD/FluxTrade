#pragma once

#include <persistence/checksum.hpp>
#include <string>
#include <fstream>
#include <filesystem>
#include <optional>
#include <cstdint>

namespace fluxtrade {
namespace persistence {

constexpr uint32_t METADATA_MAGIC = 0x4D455441; // 'M' 'E' 'T' 'A'
constexpr uint16_t METADATA_VERSION = 1;

struct alignas(8) MetadataInfo {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint64_t latest_snapshot_sequence;
    uint64_t last_journal_sequence;
    uint64_t current_journal_index;
    uint32_t checksum;
    uint32_t padding;
};
static_assert(sizeof(MetadataInfo) == 40, "MetadataInfo size must be 40 bytes");

class MetadataManager {
public:
    explicit MetadataManager(const std::string& directory) : directory_(directory) {
        std::filesystem::create_directories(directory_);
    }

    bool save(const MetadataInfo& info) const {
        std::string filepath = directory_ + "/latest.meta";
        std::string temp_filepath = filepath + ".tmp";
        
        MetadataInfo out_info = info;
        out_info.magic = METADATA_MAGIC;
        out_info.version = METADATA_VERSION;
        
        // Calculate checksum excluding the checksum and padding fields
        out_info.checksum = Checksum::crc32c(&out_info, offsetof(MetadataInfo, checksum));

        std::ofstream file(temp_filepath, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!file.is_open()) return false;
        
        file.write(reinterpret_cast<const char*>(&out_info), sizeof(MetadataInfo));
        file.flush();
        file.close();
        
        std::filesystem::rename(temp_filepath, filepath);
        return true;
    }

    std::optional<MetadataInfo> load() const {
        std::string filepath = directory_ + "/latest.meta";
        std::ifstream file(filepath, std::ios::in | std::ios::binary);
        if (!file.is_open()) return std::nullopt;
        
        MetadataInfo info{};
        file.read(reinterpret_cast<char*>(&info), sizeof(MetadataInfo));
        
        if (file.gcount() != sizeof(MetadataInfo)) return std::nullopt;
        if (info.magic != METADATA_MAGIC || info.version != METADATA_VERSION) return std::nullopt;
        
        uint32_t expected_crc = Checksum::crc32c(&info, offsetof(MetadataInfo, checksum));
        if (info.checksum != expected_crc) return std::nullopt;
        
        return info;
    }

private:
    std::string directory_;
};

} // namespace persistence
} // namespace fluxtrade
