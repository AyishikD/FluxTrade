#include <exchange/exchange.hpp>
#include <kernel/clock.hpp>

namespace fluxtrade {

Exchange::Exchange(ITransport* transport) noexcept
    : transport_(transport)
    , gateway_(transport, &inbound_queue_) {
    running_.store(false, std::memory_order_relaxed);
}

Exchange::~Exchange() {
    stop();
}

bool Exchange::start(const char* host, uint16_t port) noexcept {
    if (running_.load(std::memory_order_relaxed)) return true;
    
    // 1. Start all symbol execution loops
    for (auto& engine : engines_) {
        engine->start();
    }

    running_.store(true, std::memory_order_relaxed);

    // 2. Start outbound response thread
    response_thread_ = std::thread([this]() { run_response_loop(); });

    // 3. Start Gateway listening server
    if (!gateway_.start(host, port)) {
        stop();
        return false;
    }

    // 4. Start Gateway socket polling thread
    gateway_thread_ = std::thread([this]() {
        while (running_.load(std::memory_order_relaxed)) {
            gateway_.poll();
            
            // Dispatch commands popped from gateway queue
            OrderCommand cmd;
            while (inbound_queue_.pop(cmd)) {
                (void)dispatcher_.dispatch(cmd);
            }
            std::this_thread::yield();
        }
    });

    return true;
}

void Exchange::stop() noexcept {
    if (!running_.load(std::memory_order_relaxed)) return;
    
    running_.store(false, std::memory_order_relaxed);
    
    // Stop all symbols
    for (auto& engine : engines_) {
        engine->stop();
    }

    if (gateway_thread_.joinable()) {
        gateway_thread_.join();
    }

    if (response_thread_.joinable()) {
        response_thread_.join();
    }

    gateway_.stop();
}

void Exchange::poll() noexcept {
    gateway_.poll();
    
    OrderCommand cmd;
    while (inbound_queue_.pop(cmd)) {
        (void)dispatcher_.dispatch(cmd);
    }
}

bool Exchange::submit_command(const OrderCommand& cmd) noexcept {
    return dispatcher_.dispatch(cmd);
}

void Exchange::add_symbol(uint32_t symbol_id, uint32_t core_id) noexcept {
    const size_t slot = symbol_id % 32;
    auto& target_queue = dispatcher_.get_queue(slot);
    
    engines_.push_back(
        std::make_unique<EngineManager>(
            symbol_id,
            target_queue,
            outbound_queue_,
            core_id
        )
    );
    symbol_ids_.push_back(symbol_id);
}

void Exchange::run_response_loop() noexcept {
    OutboundReport report;
    while (running_.load(std::memory_order_relaxed)) {
        if (outbound_queue_.pop(report)) {
            if (report.type == OutboundReport::Type::ExecutionReport) {
                gateway_.send_packet(
                    report.conn_id,
                    MessageType::ExecutionReport,
                    report.data.execution.header.sequence,
                    report.data.execution.header.timestamp,
                    report.data.execution
                );
            } else if (report.type == OutboundReport::Type::Reject) {
                gateway_.send_packet(
                    report.conn_id,
                    MessageType::Reject,
                    report.data.reject.header.sequence,
                    report.data.reject.header.timestamp,
                    report.data.reject
                );
            }
        } else {
            std::this_thread::yield();
        }
    }
}

} // namespace fluxtrade
