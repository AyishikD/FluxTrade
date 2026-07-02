#pragma once

#include <cstdint>

namespace fluxtrade {

enum class EventId : uint32_t {
    OrderReceived   = 1,
    OrderAccepted   = 2,
    OrderRejected   = 3,
    OrderCancelled  = 4,
    OrderModified   = 5,
    TradeExecuted   = 6,
    TradeCancelled  = 7,
    BookUpdated     = 8,
    SnapshotCreated = 9,
    RiskRejected    = 10,
    Heartbeat       = 11,
    Startup         = 12,
    Shutdown        = 13,
    ReplayStarted   = 14,
    ReplayFinished  = 15
};

} // namespace fluxtrade
