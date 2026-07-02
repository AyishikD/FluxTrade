#pragma once

#include <events/event_id.hpp>
#include <events/event_version.hpp>
#include <kernel/module.hpp>
#include <common/types.hpp>
#include <cstdint>

namespace fluxtrade {

struct alignas(8) EventHeader {
    uint64_t timestamp = 0;
    uint64_t seq_num = 0;
    uint64_t correlation_id = 0;
    uint32_t symbol_id = 0;
    EventId id = EventId::Heartbeat;
    ModuleId source = ModuleId::Kernel;
    EventVersion version = ProtocolVersion::Current;
    uint8_t flags = 0;
    uint8_t padding[3] = {0, 0, 0};
};

static_assert(sizeof(EventHeader) == 40, "EventHeader must be exactly 40 bytes");
static_assert(alignof(EventHeader) == 8, "EventHeader must be 8-byte aligned");

} // namespace fluxtrade
