#pragma once

#include <cstdint>

namespace fluxtrade {

enum class SelfTradePreventionMode : uint8_t {
    CancelNew     = 0,
    CancelResting = 1,
    CancelBoth    = 2,
    Allow         = 3
};

struct RiskLimits {
    // Credit & Exposure configuration
    uint64_t max_credit_limit = 0;
    uint64_t max_daily_exposure = 0;

    // Fat Finger absolute limits
    uint64_t max_order_qty = 0;
    int64_t max_order_price = 0;

    // Price Band configuration (percentage, e.g. 0.10 for 10% deviation)
    double price_band_tolerance = 0.10;

    // STP configuration
    SelfTradePreventionMode stp_mode = SelfTradePreventionMode::CancelNew;

    // Order Rate Limits
    uint32_t max_orders_per_sec = 0;
    
    // Kill switch
    bool kill_switch_active = false;
};

} // namespace fluxtrade
