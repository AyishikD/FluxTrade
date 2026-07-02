#pragma once

#include <domain/price.hpp>
#include <domain/quantity.hpp>
#include <domain/order_type.hpp>
#include <domain/side.hpp>
#include <domain/time_in_force.hpp>

namespace fluxtrade {

// Validates that the price is positive
[[nodiscard]] constexpr bool is_valid_price(const Price& p) noexcept {
    return p.ticks() > 0;
}

// Validates that the quantity is positive
[[nodiscard]] constexpr bool is_valid_quantity(const Quantity& q) noexcept {
    return q.units() > 0;
}

// Validates that the order side is recognized
[[nodiscard]] constexpr bool is_valid_side(Side s) noexcept {
    return s == Side::Buy || s == Side::Sell;
}

// Validates that the order type is recognized
[[nodiscard]] constexpr bool is_valid_order_type(OrderType t) noexcept {
    return t == OrderType::Limit || t == OrderType::Market || t == OrderType::Stop;
}

// Validates that the Time in Force option is recognized
[[nodiscard]] constexpr bool is_valid_time_in_force(TimeInForce tif) noexcept {
    return tif == TimeInForce::GTC || tif == TimeInForce::IOC || tif == TimeInForce::FOK;
}

} // namespace fluxtrade
