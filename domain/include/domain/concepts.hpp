#pragma once

#include <concepts>
#include <type_traits>

namespace fluxtrade {

// Concept enforcing that an identifier is a standard layout, trivially copyable strong type
template <typename T>
concept StrongIdentifier = requires(T id) {
    { id.get() } noexcept;
} && std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

// Concept enforcing that a business object is standard layout and trivially copyable for hot-path suitability
template <typename T>
concept DomainEntity = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

} // namespace fluxtrade
