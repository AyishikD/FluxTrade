#pragma once

#include <cstdint>
#include <cstddef>

namespace fluxtrade {

class DuplicateValidator {
public:
    DuplicateValidator() noexcept {
        reset();
    }

    [[nodiscard]] bool check_duplicate(uint64_t client_order_id) noexcept {
        if (client_order_id == 0) return true;
        const size_t slot = client_order_id % MAP_SIZE;
        if (seen_ids_[slot] == client_order_id) {
            return false;
        }
        seen_ids_[slot] = client_order_id;
        return true;
    }

    void reset() noexcept {
        for (size_t i = 0; i < MAP_SIZE; ++i) {
            seen_ids_[i] = 0;
        }
    }

private:
    static constexpr size_t MAP_SIZE = 4096;
    uint64_t seen_ids_[MAP_SIZE];
};

} // namespace fluxtrade
