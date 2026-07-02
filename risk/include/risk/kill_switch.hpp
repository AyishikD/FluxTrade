#pragma once

#include <atomic>
#include <risk/risk_context.hpp>
#include <risk/risk_result.hpp>
#include <risk/risk_limits.hpp>
#include <risk/account_state.hpp>
#include <risk/position_state.hpp>

namespace fluxtrade {

class KillSwitch {
public:
    KillSwitch() = default;
    ~KillSwitch() = default;

    [[nodiscard]] static bool validate(
        const RiskContext& ctx,
        const RiskLimits& limits,
        const AccountState&,
        const PositionState&
    ) noexcept {
        if (limits.kill_switch_active || global_halt_.load(std::memory_order_relaxed)) {
            return false;
        }
        if (blocked_accounts_[ctx.account_id.get() % 1024].load(std::memory_order_relaxed)) {
            return false;
        }
        if (blocked_symbols_[ctx.symbol_id.get() % 1024].load(std::memory_order_relaxed)) {
            return false;
        }
        return true;
    }

    static void set_global_halt(bool val) noexcept {
        global_halt_.store(val, std::memory_order_relaxed);
    }

    static void set_account_halt(uint32_t account_id, bool val) noexcept {
        blocked_accounts_[account_id % 1024].store(val, std::memory_order_relaxed);
    }

    static void set_symbol_halt(uint32_t symbol_id, bool val) noexcept {
        blocked_symbols_[symbol_id % 1024].store(val, std::memory_order_relaxed);
    }

private:
    inline static std::atomic<bool> global_halt_{false};
    inline static std::atomic<bool> blocked_accounts_[1024]{};
    inline static std::atomic<bool> blocked_symbols_[1024]{};
};

} // namespace fluxtrade
