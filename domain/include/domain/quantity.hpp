#pragma once

#include <cstdint>

namespace fluxtrade {

class Quantity {
public:
    // Fixed-precision scaling factor (6 decimal places for lots/crypto fractions)
    static constexpr uint64_t SCALE = 1000000ULL;

    constexpr Quantity() : units_(0) {}
    explicit constexpr Quantity(uint64_t raw_units) : units_(raw_units) {}

    [[nodiscard]] constexpr uint64_t units() const noexcept { return units_; }

    [[nodiscard]] double to_double() const noexcept {
        return static_cast<double>(units_) / static_cast<double>(SCALE);
    }

    static constexpr Quantity from_double(double val) noexcept {
        return Quantity(static_cast<uint64_t>(val * SCALE + 0.5));
    }

    constexpr bool operator==(const Quantity& other) const noexcept { return units_ == other.units_; }
    constexpr bool operator!=(const Quantity& other) const noexcept { return units_ != other.units_; }
    constexpr bool operator<(const Quantity& other) const noexcept { return units_ < other.units_; }
    constexpr bool operator>(const Quantity& other) const noexcept { return units_ > other.units_; }
    constexpr bool operator<=(const Quantity& other) const noexcept { return units_ <= other.units_; }
    constexpr bool operator>=(const Quantity& other) const noexcept { return units_ >= other.units_; }

    constexpr Quantity operator+(const Quantity& other) const noexcept { return Quantity(units_ + other.units_); }
    constexpr Quantity operator-(const Quantity& other) const noexcept { return Quantity(units_ - other.units_); }

    Quantity& operator+=(const Quantity& other) noexcept {
        units_ += other.units_;
        return *this;
    }
    
    Quantity& operator-=(const Quantity& other) noexcept {
        units_ -= other.units_;
        return *this;
    }

private:
    uint64_t units_;
};

} // namespace fluxtrade
