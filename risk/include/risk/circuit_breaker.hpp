#pragma once

#include <risk/risk_context.hpp>
#include <risk/risk_limits.hpp>
#include <risk/account_state.hpp>
#include <risk/position_state.hpp>
#include <atomic>

namespace fluxtrade {

enum class MarketStatus : uint8_t {
    Trading        = 0,
    VolatilityHalt = 1,
    Halt           = 2,
    Auction        = 3,
    Resume         = 4
};

class CircuitBreaker {
public:
    CircuitBreaker() = default;
    ~CircuitBreaker() = default;

    [[nodiscard]] static bool validate(
        const RiskContext&,
        const RiskLimits&,
        const AccountState&,
        const PositionState&
    ) noexcept {
        const MarketStatus state = status_.load(std::memory_order_relaxed);
        if (state == MarketStatus::Halt || state == MarketStatus::VolatilityHalt) {
            return false;
        }
        return true;
    }

    static void set_status(MarketStatus state) noexcept {
        status_.store(state, std::memory_order_relaxed);
    }

    [[nodiscard]] static MarketStatus get_status() noexcept {
        return status_.load(std::memory_order_relaxed);
    }

private:
    inline static std::atomic<MarketStatus> status_{MarketStatus::Trading};
};

} // namespace fluxtrade
