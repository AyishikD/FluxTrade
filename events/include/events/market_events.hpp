#pragma once

#include <events/event_header.hpp>
#include <events/event_traits.hpp>
#include <common/types.hpp>

namespace fluxtrade {

struct alignas(8) BookUpdatedEvent {
    EventHeader header;
    Price price = 0;
    Qty qty = 0;
    Side side = Side::Buy;
    uint8_t update_type = 0; // 0 = Add, 1 = Modify, 2 = Delete
    uint8_t padding[6] = {0, 0, 0, 0, 0, 0};
};

static_assert(sizeof(BookUpdatedEvent) == 64, "BookUpdatedEvent size must be 64 bytes");
static_assert(alignof(BookUpdatedEvent) == 8, "BookUpdatedEvent alignment must be 8 bytes");

struct alignas(8) SnapshotCreatedEvent {
    EventHeader header;
    uint64_t snapshot_id = 0;
};

static_assert(sizeof(SnapshotCreatedEvent) == 48, "SnapshotCreatedEvent size must be 48 bytes");
static_assert(alignof(SnapshotCreatedEvent) == 8, "SnapshotCreatedEvent alignment must be 8 bytes");

// Trait Specializations
template <> struct is_market_event<BookUpdatedEvent> : std::true_type {};
template <> struct is_market_event<SnapshotCreatedEvent> : std::true_type {};

} // namespace fluxtrade
