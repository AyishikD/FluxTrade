#pragma once

#include <gateway/order_command.hpp>
#include <common/ring_buffer.hpp>
#include <cstddef>

namespace fluxtrade {

// Dispatcher routing orders directly to dedicated, symbol-pinned SPSC queues
template <size_t NumSymbols = 32, size_t QueueCapacity = 4096>
class Dispatcher {
public:
    Dispatcher() = default;
    ~Dispatcher() = default;

    Dispatcher(const Dispatcher&) = delete;
    Dispatcher& operator=(const Dispatcher&) = delete;

    [[nodiscard]] bool dispatch(const OrderCommand& cmd) noexcept {
        const size_t idx = cmd.symbol_id.get() % NumSymbols;
        return symbol_queues_[idx].push(cmd);
    }

    [[nodiscard]] RingBuffer<OrderCommand, QueueCapacity>& get_queue(size_t symbol_idx) noexcept {
        return symbol_queues_[symbol_idx];
    }

private:
    RingBuffer<OrderCommand, QueueCapacity> symbol_queues_[NumSymbols];
};

} // namespace fluxtrade
