#pragma once

#include <type_traits>

namespace fluxtrade {

// Base primary trait template
template <typename T>
struct is_order_event : std::false_type {};

template <typename T>
struct is_trade_event : std::false_type {};

template <typename T>
struct is_market_event : std::false_type {};

template <typename T>
struct is_system_event : std::false_type {};

template <typename T>
struct is_replay_event : std::false_type {};

template <typename T>
struct is_risk_event : std::false_type {};

// Helper constexpr constants
template <typename T>
constexpr bool is_order_event_v = is_order_event<T>::value;

template <typename T>
constexpr bool is_trade_event_v = is_trade_event<T>::value;

template <typename T>
constexpr bool is_market_event_v = is_market_event<T>::value;

template <typename T>
constexpr bool is_system_event_v = is_system_event<T>::value;

template <typename T>
constexpr bool is_replay_event_v = is_replay_event<T>::value;

template <typename T>
constexpr bool is_risk_event_v = is_risk_event<T>::value;

// C++20 Concepts for constraints
template <typename T>
concept OrderEvent = is_order_event_v<T>;

template <typename T>
concept TradeEvent = is_trade_event_v<T>;

template <typename T>
concept MarketEvent = is_market_event_v<T>;

template <typename T>
concept SystemEvent = is_system_event_v<T>;

template <typename T>
concept ReplayEvent = is_replay_event_v<T>;

template <typename T>
concept RiskEvent = is_risk_event_v<T>;

} // namespace fluxtrade
