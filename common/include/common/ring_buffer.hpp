#pragma once

#include <atomic>
#include <vector>
#include <new>
#include <cstddef>
#include <optional>
#include <utility>

namespace fluxtrade {

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_destructive_interference_size;
#else
constexpr size_t hardware_destructive_interference_size = 64;
#endif

// Bounded Lock-Free Single-Producer Single-Consumer Queue
template <typename T, size_t Capacity>
class SPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

public:
    SPSCQueue() : write_idx_(0), read_idx_(0) {
        buffer_.resize(Capacity);
    }

    ~SPSCQueue() = default;

    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;
    SPSCQueue(SPSCQueue&&) = delete;
    SPSCQueue& operator=(SPSCQueue&&) = delete;

    template <typename... Args>
    bool emplace(Args&&... args) noexcept {
        const size_t curr_write = write_idx_.load(std::memory_order_relaxed);
        const size_t curr_read = read_idx_.load(std::memory_order_acquire);

        if (curr_write - curr_read >= Capacity) {
            return false;
        }

        buffer_[curr_write & (Capacity - 1)] = T(std::forward<Args>(args)...);
        write_idx_.store(curr_write + 1, std::memory_order_release);
        return true;
    }

    bool push(const T& value) noexcept {
        return emplace(value);
    }

    bool push(T&& value) noexcept {
        const size_t curr_write = write_idx_.load(std::memory_order_relaxed);
        const size_t curr_read = read_idx_.load(std::memory_order_acquire);

        if (curr_write - curr_read >= Capacity) {
            return false;
        }

        buffer_[curr_write & (Capacity - 1)] = std::move(value);
        write_idx_.store(curr_write + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& value) noexcept {
        const size_t curr_read = read_idx_.load(std::memory_order_relaxed);
        const size_t curr_write = write_idx_.load(std::memory_order_acquire);

        if (curr_read == curr_write) {
            return false;
        }

        value = std::move(buffer_[curr_read & (Capacity - 1)]);
        read_idx_.store(curr_read + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] size_t size() const noexcept {
        const size_t curr_write = write_idx_.load(std::memory_order_relaxed);
        const size_t curr_read = read_idx_.load(std::memory_order_relaxed);
        return (curr_write >= curr_read) ? (curr_write - curr_read) : 0;
    }

    [[nodiscard]] bool empty() const noexcept {
        return size() == 0;
    }

    [[nodiscard]] size_t capacity() const noexcept {
        return Capacity;
    }

private:
    std::vector<T> buffer_;

    alignas(hardware_destructive_interference_size) std::atomic<size_t> write_idx_;
    alignas(hardware_destructive_interference_size) std::atomic<size_t> read_idx_;
};

// Alias for backwards compatibility
template <typename T, size_t Capacity>
using RingBuffer = SPSCQueue<T, Capacity>;


// Bounded Lock-Free Multi-Producer Single-Consumer Queue (Vyukov MPMC/MPSC adaptation)
template <typename T, size_t Capacity>
class MPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

public:
    MPSCQueue() : write_idx_(0), read_idx_(0) {
        for (size_t i = 0; i < Capacity; ++i) {
            buffer_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    ~MPSCQueue() = default;

    MPSCQueue(const MPSCQueue&) = delete;
    MPSCQueue& operator=(const MPSCQueue&) = delete;
    MPSCQueue(MPSCQueue&&) = delete;
    MPSCQueue& operator=(MPSCQueue&&) = delete;

    template <typename... Args>
    bool emplace(Args&&... args) noexcept {
        Cell* cell;
        size_t pos = write_idx_.load(std::memory_order_relaxed);
        for (;;) {
            cell = &buffer_[pos & (Capacity - 1)];
            size_t seq = cell->sequence.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
            if (diff == 0) {
                if (write_idx_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) {
                return false; // Queue is full
            } else {
                pos = write_idx_.load(std::memory_order_relaxed);
            }
        }

        cell->data = T(std::forward<Args>(args)...);
        cell->sequence.store(pos + 1, std::memory_order_release);
        return true;
    }

    bool push(const T& value) noexcept {
        return emplace(value);
    }

    bool push(T&& value) noexcept {
        Cell* cell;
        size_t pos = write_idx_.load(std::memory_order_relaxed);
        for (;;) {
            cell = &buffer_[pos & (Capacity - 1)];
            size_t seq = cell->sequence.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
            if (diff == 0) {
                if (write_idx_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) {
                return false; // Queue is full
            } else {
                pos = write_idx_.load(std::memory_order_relaxed);
            }
        }

        cell->data = std::move(value);
        cell->sequence.store(pos + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& value) noexcept {
        Cell* cell;
        size_t pos = read_idx_.load(std::memory_order_relaxed);
        for (;;) {
            cell = &buffer_[pos & (Capacity - 1)];
            size_t seq = cell->sequence.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);
            if (diff == 0) {
                if (read_idx_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) {
                return false; // Queue is empty
            } else {
                pos = read_idx_.load(std::memory_order_relaxed);
            }
        }

        value = std::move(cell->data);
        cell->sequence.store(pos + Capacity, std::memory_order_release);
        return true;
    }

    [[nodiscard]] size_t size() const noexcept {
        const size_t w = write_idx_.load(std::memory_order_relaxed);
        const size_t r = read_idx_.load(std::memory_order_relaxed);
        return (w >= r) ? (w - r) : 0;
    }

    [[nodiscard]] bool empty() const noexcept {
        return size() == 0;
    }

    [[nodiscard]] size_t capacity() const noexcept {
        return Capacity;
    }

private:
    struct Cell {
        std::atomic<size_t> sequence;
        T data;
    };

    // Buffer cells and indexes are aligned to prevent false sharing
    alignas(hardware_destructive_interference_size) Cell buffer_[Capacity];
    alignas(hardware_destructive_interference_size) std::atomic<size_t> write_idx_;
    alignas(hardware_destructive_interference_size) std::atomic<size_t> read_idx_;
};

} // namespace fluxtrade
