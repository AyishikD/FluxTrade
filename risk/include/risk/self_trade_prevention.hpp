#pragma once

#include <risk/risk_limits.hpp>
#include <risk/risk_context.hpp>
#include <risk/account_state.hpp>
#include <risk/position_state.hpp>

namespace fluxtrade {

class STPValidator {
public:
    [[nodiscard]] static bool validate(
        const RiskContext&,
        const RiskLimits&,
        const AccountState&,
        const PositionState&
    ) noexcept {
        // Pre-trade STP always passes since it checks aggressive vs resting at matching time.
        // This validator acts as a structural pipeline placeholder.
        return true;
    }
};

} // namespace fluxtrade
