#pragma once

#include <risk/risk_context.hpp>
#include <risk/risk_limits.hpp>
#include <risk/account_state.hpp>
#include <risk/position_state.hpp>

namespace fluxtrade {

class FatFingerValidator {
public:
    [[nodiscard]] static bool validate(
        const RiskContext& ctx,
        const RiskLimits& limits,
        const AccountState&,
        const PositionState&
    ) noexcept {
        if (limits.max_order_qty > 0 && ctx.qty.units() > limits.max_order_qty) {
            return false;
        }
        if (limits.max_order_price > 0 && ctx.price.ticks() > limits.max_order_price) {
            return false;
        }
        return true;
    }
};

} // namespace fluxtrade
