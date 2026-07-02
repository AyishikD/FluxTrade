#pragma once

#include <domain/trade.hpp>
#include <matching/execution_report.hpp>
#include <array>

namespace fluxtrade {

// Preallocated Execution Context to hold intermediate trade/report artifacts (Zero-Heap allocations)
struct ExecutionContext {
    static constexpr size_t MAX_TRADES = 128;
    static constexpr size_t MAX_REPORTS = 256;

    std::array<Trade, MAX_TRADES> trades;
    std::array<ExecutionReport, MAX_REPORTS> reports;
    size_t trade_count = 0;
    size_t report_count = 0;

    void reset() noexcept {
        trade_count = 0;
        report_count = 0;
    }

    Trade* add_trade() noexcept {
        if (trade_count >= MAX_TRADES) return nullptr;
        return &trades[trade_count++];
    }

    ExecutionReport* add_report() noexcept {
        if (report_count >= MAX_REPORTS) return nullptr;
        return &reports[report_count++];
    }
};

} // namespace fluxtrade
