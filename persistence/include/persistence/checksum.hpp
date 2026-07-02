#pragma once

#include <cstdint>
#include <cstddef>

#if defined(__aarch64__)
#include <arm_acle.h>
#elif defined(__x86_64__)
#include <nmmintrin.h>
#endif

namespace fluxtrade {
namespace persistence {

class Checksum {
public:
    static uint32_t crc32c(const void* data, size_t length, uint32_t initial_crc = 0xFFFFFFFF) noexcept {
        uint32_t crc = initial_crc;
        const uint8_t* ptr = static_cast<const uint8_t*>(data);
        
#if defined(__aarch64__) && defined(__ARM_FEATURE_CRC32)
        while (length >= 8) {
            crc = __crc32cd(crc, *reinterpret_cast<const uint64_t*>(ptr));
            ptr += 8;
            length -= 8;
        }
        while (length >= 4) {
            crc = __crc32cw(crc, *reinterpret_cast<const uint32_t*>(ptr));
            ptr += 4;
            length -= 4;
        }
        while (length > 0) {
            crc = __crc32cb(crc, *ptr);
            ptr++;
            length--;
        }
#elif defined(__x86_64__) && defined(__SSE4_2__)
        while (length >= 8) {
            crc = static_cast<uint32_t>(_mm_crc32_u64(crc, *reinterpret_cast<const uint64_t*>(ptr)));
            ptr += 8;
            length -= 8;
        }
        while (length >= 4) {
            crc = _mm_crc32_u32(crc, *reinterpret_cast<const uint32_t*>(ptr));
            ptr += 4;
            length -= 4;
        }
        while (length > 0) {
            crc = _mm_crc32_u8(crc, *ptr);
            ptr++;
            length--;
        }
#else
        // Fallback software CRC32C (Slicing-by-1 or standard bitwise)
        for (size_t i = 0; i < length; ++i) {
            crc ^= ptr[i];
            for (int j = 0; j < 8; ++j) {
                crc = (crc >> 1) ^ (0x82F63B78 * (crc & 1));
            }
        }
#endif
        return ~crc;
    }
};

} // namespace persistence
} // namespace fluxtrade
