#pragma once

#include <cstdint>

namespace fluxtrade {

class Price {
public:
    // Fixed-precision scaling factor (8 decimal places)
    static constexpr int64_t SCALE = 100000000LL;

    constexpr Price() : ticks_(0) {}
    explicit constexpr Price(int64_t raw_ticks) : ticks_(raw_ticks) {}

    [[nodiscard]] constexpr int64_t ticks() const noexcept { return ticks_; }

    [[nodiscard]] double to_double() const noexcept {
        return static_cast<double>(ticks_) / static_cast<double>(SCALE);
    }

    static constexpr Price from_double(double val) noexcept {
        return Price(static_cast<int64_t>(val * SCALE + (val >= 0.0 ? 0.5 : -0.5)));
    }

    constexpr bool operator==(const Price& other) const noexcept { return ticks_ == other.ticks_; }
    constexpr bool operator!=(const Price& other) const noexcept { return ticks_ != other.ticks_; }
    constexpr bool operator<(const Price& other) const noexcept { return ticks_ < other.ticks_; }
    constexpr bool operator>(const Price& other) const noexcept { return ticks_ > other.ticks_; }
    constexpr bool operator<=(const Price& other) const noexcept { return ticks_ <= other.ticks_; }
    constexpr bool operator>=(const Price& other) const noexcept { return ticks_ >= other.ticks_; }

    constexpr Price operator+(const Price& other) const noexcept { return Price(ticks_ + other.ticks_); }
    constexpr Price operator-(const Price& other) const noexcept { return Price(ticks_ - other.ticks_); }

    Price& operator+=(const Price& other) noexcept {
        ticks_ += other.ticks_;
        return *this;
    }
    
    Price& operator-=(const Price& other) noexcept {
        ticks_ -= other.ticks_;
        return *this;
    }

private:
    int64_t ticks_;
};

} // namespace fluxtrade
