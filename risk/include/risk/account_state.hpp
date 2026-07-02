#pragma once

#include <cstdint>

namespace fluxtrade {

// Mutable Account Balance and Margin State (Exactly 64 Bytes, Fits inside a single CPU cache line)
struct alignas(8) AccountState {
    uint64_t available_buying_power = 0;
    uint64_t used_buying_power = 0;
    uint64_t reserved_capital = 0;
    uint64_t daily_exposure = 0;
    
    uint64_t worst_case_loss = 0;
    uint64_t open_margin_requirement = 0;
    uint64_t collateral_value = 0;
    uint64_t account_id = 0; // Wraps AccountId underlying get()
};

static_assert(sizeof(AccountState) == 64, "AccountState size must be exactly 64 bytes");
static_assert(alignof(AccountState) == 8, "AccountState alignment must be 8 bytes");

} // namespace fluxtrade
