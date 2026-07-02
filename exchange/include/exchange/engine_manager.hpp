#pragma once

#include <gateway/order_command.hpp>
#include <exchange/outbound_report.hpp>
#include <matching/matching_engine.hpp>
#include <risk/risk_engine.hpp>
#include <common/ring_buffer.hpp>
#include <common/thread_pinning.hpp>
#include <atomic>
#include <thread>
#include <chrono>
#include <persistence/memory_journal.hpp>
#include <persistence/disk_journal.hpp>

namespace fluxtrade {

class EngineManager {
public:
    EngineManager(
        uint32_t symbol_id,
        RingBuffer<OrderCommand, 4096>& inbound_queue,
        RingBuffer<OutboundReport, 4096>& outbound_queue,
        uint32_t core_id = 0
    ) noexcept
        : symbol_id_(symbol_id)
        , inbound_queue_(inbound_queue)
        , outbound_queue_(outbound_queue)
        , core_id_(core_id)
    {
        std::string dir = "wal/symbol_" + std::to_string(symbol_id);
        disk_journal_ = std::make_unique<persistence::DiskJournal>(dir, persistence::FlushPolicy::Batch);
    }

    ~EngineManager() {
        stop();
    }

    EngineManager(const EngineManager&) = delete;
    EngineManager& operator=(const EngineManager&) = delete;

    void start() noexcept {
        if (running_.load(std::memory_order_relaxed)) return;
        running_.store(true, std::memory_order_relaxed);
        disk_journal_->start(journal_queue_, core_id_ > 0 ? core_id_ + 1 : 0);
        thread_ = std::thread([this]() { run(); });
    }

    void stop() noexcept {
        if (!running_.load(std::memory_order_relaxed)) return;
        running_.store(false, std::memory_order_relaxed);
        disk_journal_->stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    [[nodiscard]] MatchingEngine& matching() noexcept { return matching_engine_; }
    [[nodiscard]] RiskEngine& risk() noexcept { return risk_engine_; }

private:
    void run() noexcept {
        if (core_id_ > 0) {
            pin_thread(core_id_);
        }

        OrderCommand cmd;
        while (running_.load(std::memory_order_relaxed)) {
            if (inbound_queue_.pop(cmd)) {
                process_command(cmd);
            } else {
                std::this_thread::yield();
            }
        }
    }

    void process_command(const OrderCommand& cmd) noexcept {
        // 1. Construct RiskContext
        RiskContext r_ctx;
        r_ctx.order_id = OrderId(cmd.client_order_id);
        r_ctx.price = cmd.price;
        r_ctx.qty = cmd.qty;
        r_ctx.timestamp = cmd.timestamp;
        r_ctx.reference_price = Price::from_double(100.00); // base close
        r_ctx.client_order_id = cmd.client_order_id;
        r_ctx.symbol_id = cmd.symbol_id;
        r_ctx.client_id = ClientId(1);
        r_ctx.account_id = cmd.account_id;
        r_ctx.side = cmd.side;
        r_ctx.type = cmd.order_type;

        // 2. Perform Pre-Trade Risk Checks
        RiskTrace trace;
        auto r_res = risk_engine_.validate(r_ctx, trace);

        if (r_res.has_value()) {
            // Risk validation passed! Append to WAL
            persistence::MemoryJournal::append(
                journal_queue_,
                cmd.sequence,
                cmd.timestamp,
                static_cast<uint16_t>(cmd.type),
                &cmd,
                sizeof(OrderCommand)
            );

            // Forward command to the Order Book / Matching Engine
            if (cmd.type == CommandType::NewOrder) {
                Order order{
                    .order_id = OrderId(cmd.client_order_id),
                    .price = cmd.price,
                    .qty = cmd.qty,
                    .seq_num = cmd.sequence,
                    .client_timestamp = cmd.timestamp,
                    .exchange_timestamp = Clock::now_steady(),
                    .client_id = ClientId(1),
                    .account_id = cmd.account_id,
                    .symbol_id = cmd.symbol_id,
                    .side = cmd.side,
                    .type = cmd.order_type,
                    .tif = cmd.tif,
                    .flags = OrderFlags::None
                };

                exec_ctx_.reset();
                matching_engine_.submit_order(order, exec_ctx_);
                dispatch_reports(cmd.conn_id, cmd.side);
            } else if (cmd.type == CommandType::CancelOrder) {
                exec_ctx_.reset();
                matching_engine_.cancel_order(
                    OrderId(cmd.orig_client_order_id),
                    ClientId(1),
                    cmd.account_id,
                    cmd.symbol_id,
                    exec_ctx_
                );
                dispatch_reports(cmd.conn_id, Side::Buy); // side is unused in cancellation report mappings
            }
        } else {
            // Risk validation failed! Reject immediately
            OutboundReport report;
            report.type = OutboundReport::Type::Reject;
            report.conn_id = cmd.conn_id;
            report.data.reject.header.length = sizeof(RejectPacket);
            report.data.reject.header.type = static_cast<uint16_t>(MessageType::Reject);
            report.data.reject.header.sequence = cmd.sequence;
            report.data.reject.header.timestamp = Clock::now_steady();
            report.data.reject.client_order_id = cmd.client_order_id;
            report.data.reject.account_id = cmd.account_id.get();
            report.data.reject.reason = static_cast<uint16_t>(r_res.error());

            outbound_queue_.push(report);
        }
    }

    void dispatch_reports(int32_t conn_id, Side side) noexcept {
        for (size_t i = 0; i < exec_ctx_.report_count; ++i) {
            const auto& engine_report = exec_ctx_.reports[i];
            
            OutboundReport out_report;
            out_report.type = OutboundReport::Type::ExecutionReport;
            out_report.conn_id = conn_id;

            auto& packet = out_report.data.execution;
            packet.header.length = sizeof(ExecutionReportPacket);
            packet.header.type = static_cast<uint16_t>(MessageType::ExecutionReport);
            packet.header.sequence = engine_report.exec_id.get();
            packet.header.timestamp = engine_report.timestamp;

            packet.order_id = engine_report.order_id.get();
            packet.client_order_id = engine_report.order_id.get(); // local mirror
            packet.symbol_id = engine_report.symbol_id.get();
            packet.account_id = engine_report.account_id.get();
            packet.price_ticks = engine_report.price.ticks();
            packet.qty_units = engine_report.last_qty.units() + engine_report.remaining_qty.units();
            packet.filled_qty = engine_report.last_qty.units();
            packet.remaining_qty = engine_report.remaining_qty.units();
            packet.side = side == Side::Buy ? 0 : 1;
            packet.status = static_cast<uint8_t>(engine_report.exec_type);

            outbound_queue_.push(out_report);

            // Feed filled updates back to Risk Engine to manage dynamic exposures/margins
            if (engine_report.exec_type == ExecutionType::Trade) {
                risk_engine_.update_on_fill(
                    engine_report.account_id.get(),
                    engine_report.symbol_id.get(),
                    side,
                    engine_report.last_qty.units(),
                    engine_report.price.ticks()
                );
            } else if (engine_report.exec_type == ExecutionType::Cancel) {
                const uint64_t cost = (engine_report.price.ticks() * engine_report.last_qty.units()) / 1000000ULL;
                risk_engine_.update_on_cancel(engine_report.account_id.get(), cost);
            }
        }
    }

    [[maybe_unused]] uint32_t symbol_id_;
    RingBuffer<OrderCommand, 4096>& inbound_queue_;
    RingBuffer<OutboundReport, 4096>& outbound_queue_;
    RingBuffer<persistence::LogEntry, 65536> journal_queue_;
    std::unique_ptr<persistence::DiskJournal> disk_journal_;
    uint32_t core_id_;
    
    std::atomic<bool> running_{false};
    std::thread thread_;
    MatchingEngine matching_engine_;
    RiskEngine risk_engine_;
    ExecutionContext exec_ctx_;
};

} // namespace fluxtrade
