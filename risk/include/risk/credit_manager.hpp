#pragma once

#include <risk/account_state.hpp>
#include <risk/risk_context.hpp>
#include <risk/risk_limits.hpp>
#include <risk/position_state.hpp>
#include <cstddef>

namespace fluxtrade {

class CreditManager {
public:
    CreditManager() noexcept;
    ~CreditManager() = default;

    CreditManager(const CreditManager&) = delete;
    CreditManager& operator=(const CreditManager&) = delete;

    [[nodiscard]] static bool validate(
        const RiskContext& ctx,
        const RiskLimits& limits,
        const AccountState& account,
        const PositionState&
    ) noexcept {
        const uint64_t cost = (ctx.price.ticks() * ctx.qty.units()) / 1000000ULL;
        if (cost > account.available_buying_power) {
            return false;
        }
        if (limits.max_daily_exposure > 0 && (account.daily_exposure + cost) > limits.max_daily_exposure) {
            return false;
        }
        return true;
    }

    [[nodiscard]] AccountState* get_account_state(uint32_t account_id) noexcept;
    
    void update_on_accept(uint32_t account_id, uint64_t cost) noexcept;
    void update_on_fill(uint32_t account_id, uint64_t cost) noexcept;
    void update_on_cancel(uint32_t account_id, uint64_t cost) noexcept;

    void reset() noexcept;

private:
    static constexpr size_t MAX_ACCOUNTS = 1024;
    AccountState accounts_[MAX_ACCOUNTS];
};

} // namespace fluxtrade
