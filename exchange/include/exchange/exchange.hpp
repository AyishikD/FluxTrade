#pragma once

#include <gateway/gateway.hpp>
#include <exchange/dispatcher.hpp>
#include <exchange/engine_manager.hpp>
#include <exchange/outbound_report.hpp>
#include <common/ring_buffer.hpp>
#include <atomic>
#include <thread>
#include <vector>
#include <memory>

namespace fluxtrade {

class Exchange {
public:
    explicit Exchange(ITransport* transport) noexcept;
    ~Exchange();

    Exchange(const Exchange&) = delete;
    Exchange& operator=(const Exchange&) = delete;

    bool start(const char* host, uint16_t port) noexcept;
    void stop() noexcept;
    void poll() noexcept;

    // Direct interface injections for simulator/tests bypass
    bool submit_command(const OrderCommand& cmd) noexcept;

    void add_symbol(uint32_t symbol_id, uint32_t core_id = 0) noexcept;
    
    [[nodiscard]] Gateway& gateway() noexcept { return gateway_; }
    [[nodiscard]] Dispatcher<32, 4096>& dispatcher() noexcept { return dispatcher_; }
    [[nodiscard]] std::vector<std::unique_ptr<EngineManager>>& engines() noexcept { return engines_; }

private:
    void run_response_loop() noexcept;

    [[maybe_unused]] ITransport* transport_;
    RingBuffer<OrderCommand, 4096> inbound_queue_;
    RingBuffer<OutboundReport, 4096> outbound_queue_;
    Gateway gateway_;
    Dispatcher<32, 4096> dispatcher_;
    
    std::vector<std::unique_ptr<EngineManager>> engines_;
    std::vector<uint32_t> symbol_ids_;
    
    std::atomic<bool> running_{false};
    std::thread response_thread_;
    std::thread gateway_thread_;
};

} // namespace fluxtrade
