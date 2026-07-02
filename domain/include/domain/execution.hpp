#pragma once

#include <domain/ids.hpp>
#include <domain/price.hpp>
#include <domain/quantity.hpp>
#include <domain/execution_type.hpp>

namespace fluxtrade {

struct alignas(8) Execution {
    ExecutionId execution_id;
    OrderId order_id;
    Price price;
    Quantity executed_qty;
    Quantity remaining_qty;
    uint64_t timestamp = 0;
    ExecutionType exec_type = ExecutionType::New;
    uint8_t padding[7] = {0, 0, 0, 0, 0, 0, 0}; // Pad to 8 bytes boundary
};

static_assert(sizeof(Execution) == 56, "Execution size must be exactly 56 bytes");
static_assert(alignof(Execution) == 8, "Execution alignment must be 8 bytes");

} // namespace fluxtrade
