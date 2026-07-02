#pragma once

#include <events/event_header.hpp>
#include <events/event_traits.hpp>
#include <common/types.hpp>

namespace fluxtrade {

struct alignas(8) RiskRejectedEvent {
    EventHeader header;
    OrderId order_id = 0;
    ClientId client_id = 0;
    uint32_t risk_rule_id = 0;
};

static_assert(sizeof(RiskRejectedEvent) == 56, "RiskRejectedEvent size must be 56 bytes");
static_assert(alignof(RiskRejectedEvent) == 8, "RiskRejectedEvent alignment must be 8 bytes");

// Trait Specializations
template <> struct is_risk_event<RiskRejectedEvent> : std::true_type {};

} // namespace fluxtrade
