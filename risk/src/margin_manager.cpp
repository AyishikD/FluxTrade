#include <risk/margin_manager.hpp>

namespace fluxtrade {

MarginManager::MarginManager() noexcept {
    reset();
}

PositionState* MarginManager::get_position_state(uint32_t account_id, uint32_t symbol_id) noexcept {
    PositionState* state = &positions_[account_id % MAX_ACCOUNTS][symbol_id % MAX_SYMBOLS];
    if (state->account_id == 0 && state->symbol_id == 0) {
        state->account_id = account_id;
        state->symbol_id = symbol_id;
    }
    return state;
}

void MarginManager::update_on_fill(uint32_t account_id, uint32_t symbol_id, Side side, uint64_t qty) noexcept {
    PositionState* state = get_position_state(account_id, symbol_id);
    if (side == Side::Buy) {
        state->long_qty += qty;
    } else {
        state->short_qty += qty;
    }
    state->net_qty = static_cast<int64_t>(state->long_qty) - static_cast<int64_t>(state->short_qty);
    state->gross_qty = state->long_qty + state->short_qty;
}

void MarginManager::reset() noexcept {
    for (size_t a = 0; a < MAX_ACCOUNTS; ++a) {
        for (size_t s = 0; s < MAX_SYMBOLS; ++s) {
            positions_[a][s].account_id = static_cast<uint32_t>(a);
            positions_[a][s].symbol_id = static_cast<uint32_t>(s);
            positions_[a][s].net_qty = 0;
            positions_[a][s].long_qty = 0;
            positions_[a][s].short_qty = 0;
            positions_[a][s].gross_qty = 0;
            // Default position limits to 1,000,000,000 units (1,000 shares)
            positions_[a][s].max_long_limit = 1000000000ULL;
            positions_[a][s].max_short_limit = 1000000000ULL;
            positions_[a][s].last_update_ts = 0;
        }
    }
}

} // namespace fluxtrade
