#pragma once

#include <domain/ids.hpp>
#include <domain/price.hpp>
#include <domain/quantity.hpp>
#include <domain/execution_type.hpp>
#include <domain/reject_reason.hpp>

namespace fluxtrade {

// Client-facing Execution Report (64 Bytes, Fits exactly inside a single CPU cache line)
struct alignas(8) ExecutionReport {
    // 8-byte members (48 bytes)
    ExecutionId exec_id;
    OrderId order_id;
    Price price;
    Quantity last_qty;
    Quantity remaining_qty;
    uint64_t timestamp = 0;

    // 4-byte members (12 bytes)
    ClientId client_id;
    AccountId account_id;
    SymbolId symbol_id;

    // 2-byte members (2 bytes)
    RejectReason reject_reason = RejectReason::None;

    // 1-byte members (1 byte)
    ExecutionType exec_type = ExecutionType::New;

    // 1-byte padding to align to 8-byte boundary (1 byte)
    uint8_t padding = 0;
};

static_assert(sizeof(ExecutionReport) == 64, "ExecutionReport size must be exactly 64 bytes");
static_assert(alignof(ExecutionReport) == 8, "ExecutionReport alignment must be 8 bytes");

} // namespace fluxtrade
