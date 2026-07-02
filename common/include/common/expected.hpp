#pragma once

#include <variant>
#include <type_traits>
#include <stdexcept>
#include <utility>

namespace fluxtrade {

template <typename E>
class unexpected {
public:
    template <typename Err = E>
    requires std::is_constructible_v<E, Err>
    explicit constexpr unexpected(Err&& error) : error_(std::forward<Err>(error)) {}

    constexpr const E& error() const noexcept { return error_; }
    constexpr E& error() noexcept { return error_; }

private:
    E error_;
};

template <typename T, typename E>
class expected {
public:
    constexpr expected() requires std::is_default_constructible_v<T> : data_(T{}) {}
    
    constexpr expected(const T& value) : data_(value) {}
    constexpr expected(T&& value) : data_(std::move(value)) {}

    template <typename Err>
    requires std::is_constructible_v<E, Err>
    constexpr expected(const unexpected<Err>& unexp) : data_(unexpected<E>(unexp.error())) {}

    template <typename Err>
    requires std::is_constructible_v<E, Err>
    constexpr expected(unexpected<Err>&& unexp) : data_(unexpected<E>(std::move(unexp.error()))) {}

    constexpr bool has_value() const noexcept {
        return data_.index() == 0;
    }

    constexpr explicit operator bool() const noexcept {
        return has_value();
    }

    constexpr const T& value() const {
        if (!has_value()) {
            throw std::bad_alloc();
        }
        return std::get<0>(data_);
    }

    constexpr T& value() {
        if (!has_value()) {
            throw std::bad_alloc();
        }
        return std::get<0>(data_);
    }

    constexpr const T& operator*() const noexcept { return std::get<0>(data_); }
    constexpr T& operator*() noexcept { return std::get<0>(data_); }

    constexpr const T* operator->() const noexcept { return &std::get<0>(data_); }
    constexpr T* operator->() noexcept { return &std::get<0>(data_); }

    constexpr const E& error() const {
        return std::get<1>(data_).error();
    }

    constexpr E& error() {
        return std::get<1>(data_).error();
    }

private:
    std::variant<T, unexpected<E>> data_;
};

// Specialization for expected<void, E>
template <typename E>
class expected<void, E> {
public:
    constexpr expected() : has_val_(true) {}

    template <typename Err>
    requires std::is_constructible_v<E, Err>
    constexpr expected(const unexpected<Err>& unexp) : data_(unexpected<E>(unexp.error())), has_val_(false) {}

    template <typename Err>
    requires std::is_constructible_v<E, Err>
    constexpr expected(unexpected<Err>&& unexp) : data_(unexpected<E>(std::move(unexp.error()))), has_val_(false) {}

    constexpr bool has_value() const noexcept {
        return has_val_;
    }

    constexpr explicit operator bool() const noexcept {
        return has_value();
    }

    constexpr void value() const {
        if (!has_val_) {
            throw std::bad_alloc();
        }
    }

    constexpr const E& error() const {
        return data_.error();
    }

    constexpr E& error() {
        return data_.error();
    }

private:
    unexpected<E> data_{E{}};
    bool has_val_;
};

} // namespace fluxtrade
