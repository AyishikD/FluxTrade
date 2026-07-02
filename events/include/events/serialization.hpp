#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace fluxtrade {

// Returns compile-time static size of trivially copyable type
template <typename T>
[[nodiscard]] constexpr size_t serialized_size() noexcept {
    static_assert(std::is_trivially_copyable_v<T>, "Event must be trivially copyable for binary serialization size query");
    return sizeof(T);
}

// Serializes trivially copyable event directly into a raw binary byte buffer
template <typename T>
[[nodiscard]] size_t serialize(const T& event, uint8_t* buffer, size_t size) noexcept {
    static_assert(std::is_trivially_copyable_v<T>, "Event must be trivially copyable for binary serialization");
    const size_t needed = sizeof(T);
    if (size < needed) {
        return 0;
    }
    std::memcpy(buffer, &event, needed);
    return needed;
}

// Deserializes a raw binary byte buffer directly into a trivially copyable event struct
template <typename T>
[[nodiscard]] bool deserialize(const uint8_t* buffer, size_t size, T& event) noexcept {
    static_assert(std::is_trivially_copyable_v<T>, "Event must be trivially copyable for binary deserialization");
    const size_t needed = sizeof(T);
    if (size < needed) {
        return false;
    }
    std::memcpy(&event, buffer, needed);
    return true;
}

} // namespace fluxtrade
