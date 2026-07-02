#pragma once

#include <cstdint>

namespace fluxtrade {

// Compile-time zero-overhead strong type wrapper
template <typename Tag, typename Underlying = uint64_t>
class StrongId {
public:
    constexpr StrongId() : value_(0) {}
    explicit constexpr StrongId(Underlying val) : value_(val) {}

    [[nodiscard]] constexpr Underlying get() const noexcept { return value_; }

    constexpr bool operator==(const StrongId& other) const noexcept { return value_ == other.value_; }
    constexpr bool operator!=(const StrongId& other) const noexcept { return value_ != other.value_; }
    constexpr bool operator<(const StrongId& other) const noexcept { return value_ < other.value_; }
    constexpr bool operator>(const StrongId& other) const noexcept { return value_ > other.value_; }

private:
    Underlying value_;
};

// Tag types for specialization
struct OrderIdTag {};
using OrderId = StrongId<OrderIdTag, uint64_t>;

struct TradeIdTag {};
using TradeId = StrongId<TradeIdTag, uint64_t>;

struct ExecutionIdTag {};
using ExecutionId = StrongId<ExecutionIdTag, uint64_t>;

struct FillIdTag {};
using FillId = StrongId<FillIdTag, uint64_t>;

struct ClientIdTag {};
using ClientId = StrongId<ClientIdTag, uint32_t>;

struct AccountIdTag {};
using AccountId = StrongId<AccountIdTag, uint32_t>;

struct SymbolIdTag {};
using SymbolId = StrongId<SymbolIdTag, uint32_t>;

// Combined Symbol ID and Order ID for indexing in the matching engine and router maps
struct OrderKey {
    SymbolId symbol_id;
    OrderId order_id;

    constexpr bool operator==(const OrderKey& other) const noexcept {
        return symbol_id == other.symbol_id && order_id == other.order_id;
    }
    
    constexpr bool operator!=(const OrderKey& other) const noexcept {
        return !(*this == other);
    }
    
    constexpr bool operator<(const OrderKey& other) const noexcept {
        if (symbol_id != other.symbol_id) {
            return symbol_id < other.symbol_id;
        }
        return order_id < other.order_id;
    }
};

} // namespace fluxtrade
