#pragma once

#include <cstdint>
#include <cstddef>

namespace fluxtrade {

class RateLimitValidator {
public:
    RateLimitValidator() noexcept {
        reset();
    }

    [[nodiscard]] bool check_rate(uint32_t account_id, uint64_t now, uint32_t limit) noexcept {
        if (limit == 0) return true;
        
        auto& tr = trackers_[account_id % MAX_ACCOUNTS];
        
        if (tr.count < limit) {
            tr.timestamps[tr.index] = now;
            tr.index = (tr.index + 1) % Tracker::WINDOW_SIZE;
            tr.count++;
            return true;
        }
        
        // Calculate the circular index of the oldest message for this dynamic limit
        const size_t oldest_index = (tr.index + Tracker::WINDOW_SIZE - limit) % Tracker::WINDOW_SIZE;
        const uint64_t oldest = tr.timestamps[oldest_index];
        if (now - oldest < 1000000000ULL) { // 1 second sliding window in ns
            return false;
        }
        
        tr.timestamps[tr.index] = now;
        tr.index = (tr.index + 1) % Tracker::WINDOW_SIZE;
        return true;
    }

    void reset() noexcept {
        for (size_t a = 0; a < MAX_ACCOUNTS; ++a) {
            trackers_[a].index = 0;
            trackers_[a].count = 0;
            for (size_t i = 0; i < Tracker::WINDOW_SIZE; ++i) {
                trackers_[a].timestamps[i] = 0;
            }
        }
    }

private:
    struct Tracker {
        static constexpr size_t WINDOW_SIZE = 100;
        uint64_t timestamps[WINDOW_SIZE] = {0};
        size_t index = 0;
        size_t count = 0;
    };
    static constexpr size_t MAX_ACCOUNTS = 1024;
    Tracker trackers_[MAX_ACCOUNTS];
};

} // namespace fluxtrade
