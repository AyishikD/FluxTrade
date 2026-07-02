#pragma once

#include <risk/position_state.hpp>
#include <risk/risk_context.hpp>
#include <risk/risk_limits.hpp>
#include <risk/account_state.hpp>
#include <cstddef>

namespace fluxtrade {

class MarginManager {
public:
    MarginManager() noexcept;
    ~MarginManager() = default;

    MarginManager(const MarginManager&) = delete;
    MarginManager& operator=(const MarginManager&) = delete;

    [[nodiscard]] static bool validate(
        const RiskContext& ctx,
        const RiskLimits&,
        const AccountState&,
        const PositionState& position
    ) noexcept {
        const int64_t trade_qty = ctx.qty.units();
        
        int64_t next_net = position.net_qty;
        if (ctx.side == Side::Buy) {
            next_net += trade_qty;
        } else {
            next_net -= trade_qty;
        }

        if (next_net > 0 && position.max_long_limit > 0 && static_cast<uint64_t>(next_net) > position.max_long_limit) {
            return false;
        }
        if (next_net < 0 && position.max_short_limit > 0 && static_cast<uint64_t>(-next_net) > position.max_short_limit) {
            return false;
        }
        return true;
    }

    [[nodiscard]] PositionState* get_position_state(uint32_t account_id, uint32_t symbol_id) noexcept;

    void update_on_fill(uint32_t account_id, uint32_t symbol_id, Side side, uint64_t qty) noexcept;
    
    void reset() noexcept;

private:
    static constexpr size_t MAX_ACCOUNTS = 128;
    static constexpr size_t MAX_SYMBOLS = 32;
    PositionState positions_[MAX_ACCOUNTS][MAX_SYMBOLS];
};

} // namespace fluxtrade
