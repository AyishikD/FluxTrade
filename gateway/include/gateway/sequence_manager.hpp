#pragma once

#include <atomic>
#include <cstdint>

namespace fluxtrade {

class SequenceManager {
public:
    SequenceManager() noexcept = default;
    ~SequenceManager() = default;

    [[nodiscard]] uint64_t next_sequence() noexcept {
        return seq_.fetch_add(1, std::memory_order_relaxed);
    }

    void reset(uint64_t start = 1) noexcept {
        seq_.store(start, std::memory_order_relaxed);
    }

    [[nodiscard]] uint64_t current() const noexcept {
        return seq_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<uint64_t> seq_{1};
};

} // namespace fluxtrade
