#pragma once

#include <cstdint>

namespace fluxtrade {

enum class OrderFlags : uint8_t {
    None       = 0,
    PostOnly   = 1 << 0,
    ReduceOnly = 1 << 1
};

// Bitwise helper operators for OrderFlags
constexpr OrderFlags operator|(OrderFlags a, OrderFlags b) noexcept {
    return static_cast<OrderFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

constexpr OrderFlags operator&(OrderFlags a, OrderFlags b) noexcept {
    return static_cast<OrderFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

constexpr bool has_flag(OrderFlags flags, OrderFlags flag) noexcept {
    return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(flag)) != 0;
}

} // namespace fluxtrade
