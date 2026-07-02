#pragma once

#include <domain/ids.hpp>
#include <domain/price.hpp>
#include <domain/quantity.hpp>
#include <domain/side.hpp>
#include <domain/order_type.hpp>
#include <risk/risk_result.hpp>
#include <cstdint>
#include <cstddef>

namespace fluxtrade {

// Pre-Trade Risk Validation Context (Exactly 64 Bytes, Fits inside a single CPU cache line)
struct alignas(8) RiskContext {
    // 8-byte members (48 bytes total)
    OrderId order_id;
    Price price;
    Quantity qty;
    uint64_t timestamp = 0;
    Price reference_price;
    uint64_t client_order_id = 0;

    // 4-byte members (12 bytes total)
    SymbolId symbol_id;
    ClientId client_id;
    AccountId account_id;

    // 1-byte members (2 bytes total)
    Side side = Side::Buy;
    OrderType type = OrderType::Limit;

    // 2-byte alignment padding (2 bytes total)
    uint8_t padding[2] = {0, 0};
};

static_assert(sizeof(RiskContext) == 64, "RiskContext size must be exactly 64 bytes");
static_assert(alignof(RiskContext) == 8, "RiskContext alignment must be 8 bytes");

// Lightweight audit trail stage definition (used to compile execution trace records)
struct RiskTraceStage {
    char stage_name[24] = {0};
    uint32_t latency_ns = 0;
    RiskDecision decision = RiskDecision::Accept;
    uint8_t padding[3] = {0, 0, 0};
};

struct RiskTrace {
    static constexpr size_t MAX_STAGES = 16;
    RiskTraceStage stages[MAX_STAGES];
    size_t stage_count = 0;

    void add_stage(const char* name, uint32_t latency, RiskDecision dec) noexcept {
        if (stage_count >= MAX_STAGES) return;
        auto& stage = stages[stage_count++];
        size_t i = 0;
        while (i < 23 && name[i] != '\0') {
            stage.stage_name[i] = name[i];
            i++;
        }
        stage.stage_name[i] = '\0';
        stage.latency_ns = latency;
        stage.decision = dec;
    }
};

} // namespace fluxtrade
