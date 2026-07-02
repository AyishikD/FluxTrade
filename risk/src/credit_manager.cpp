#include <risk/credit_manager.hpp>

namespace fluxtrade {

CreditManager::CreditManager() noexcept {
    reset();
}

AccountState* CreditManager::get_account_state(uint32_t account_id) noexcept {
    AccountState* state = &accounts_[account_id % MAX_ACCOUNTS];
    if (state->account_id == 0) {
        state->account_id = account_id;
    }
    return state;
}

void CreditManager::update_on_accept(uint32_t account_id, uint64_t cost) noexcept {
    AccountState* state = get_account_state(account_id);
    if (state->available_buying_power >= cost) {
        state->available_buying_power -= cost;
        state->reserved_capital += cost;
    }
}

void CreditManager::update_on_fill(uint32_t account_id, uint64_t cost) noexcept {
    AccountState* state = get_account_state(account_id);
    if (state->reserved_capital >= cost) {
        state->reserved_capital -= cost;
    } else {
        uint64_t remainder = cost - state->reserved_capital;
        state->reserved_capital = 0;
        if (state->available_buying_power >= remainder) {
            state->available_buying_power -= remainder;
        }
    }
    state->used_buying_power += cost;
    state->daily_exposure += cost;
}

void CreditManager::update_on_cancel(uint32_t account_id, uint64_t cost) noexcept {
    AccountState* state = get_account_state(account_id);
    if (state->reserved_capital >= cost) {
        state->reserved_capital -= cost;
        state->available_buying_power += cost;
    }
}

void CreditManager::reset() noexcept {
    for (size_t i = 0; i < MAX_ACCOUNTS; ++i) {
        accounts_[i].account_id = i;
        // Default initial buying power to 1,000,000,000,000,000 ticks ($10 million)
        accounts_[i].available_buying_power = 1000000000000000ULL;
        accounts_[i].used_buying_power = 0;
        accounts_[i].reserved_capital = 0;
        accounts_[i].daily_exposure = 0;
        accounts_[i].worst_case_loss = 0;
        accounts_[i].open_margin_requirement = 0;
        accounts_[i].collateral_value = 0;
    }
}

} // namespace fluxtrade
