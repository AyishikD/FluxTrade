#pragma once

#include <events/event_header.hpp>
#include <type_traits>
#include <concepts>

namespace fluxtrade {

// Concept enforcing that a type contains a standard EventHeader as its first member,
// is trivially copyable, and satisfies standard layout requirements.
template <typename T>
concept ExchangeEvent = requires(T event) {
    { event.header } -> std::same_as<EventHeader&>;
} && std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

} // namespace fluxtrade
