#pragma once

#include <risk/risk_context.hpp>
#include <risk/risk_limits.hpp>
#include <risk/account_state.hpp>
#include <risk/position_state.hpp>

namespace fluxtrade {

class PriceBandValidator {
public:
    [[nodiscard]] static bool validate(
        const RiskContext& ctx,
        const RiskLimits& limits,
        const AccountState&,
        const PositionState&
    ) noexcept {
        if (ctx.reference_price.ticks() == 0) {
            return true; // No reference price registered (allow matching)
        }
        
        const double ref = ctx.reference_price.to_double();
        const double tolerance = limits.price_band_tolerance;
        
        const double upper_limit = ref * (1.0 + tolerance);
        const double lower_limit = ref * (1.0 - tolerance);
        
        const double order_price = ctx.price.to_double();
        
        if (order_price > upper_limit || order_price < lower_limit) {
            return false;
        }
        return true;
    }
};

} // namespace fluxtrade
