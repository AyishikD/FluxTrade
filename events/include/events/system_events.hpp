#pragma once

#include <events/event_header.hpp>
#include <events/event_traits.hpp>

namespace fluxtrade {

struct alignas(8) HeartbeatEvent {
    EventHeader header;
};
static_assert(sizeof(HeartbeatEvent) == 40, "HeartbeatEvent size must be 40 bytes");
static_assert(alignof(HeartbeatEvent) == 8, "HeartbeatEvent alignment must be 8 bytes");

struct alignas(8) StartupEvent {
    EventHeader header;
};
static_assert(sizeof(StartupEvent) == 40, "StartupEvent size must be 40 bytes");
static_assert(alignof(StartupEvent) == 8, "StartupEvent alignment must be 8 bytes");

struct alignas(8) ShutdownEvent {
    EventHeader header;
};
static_assert(sizeof(ShutdownEvent) == 40, "ShutdownEvent size must be 40 bytes");
static_assert(alignof(ShutdownEvent) == 8, "ShutdownEvent alignment must be 8 bytes");

struct alignas(8) ReplayStartedEvent {
    EventHeader header;
    uint64_t start_sequence = 0;
};
static_assert(sizeof(ReplayStartedEvent) == 48, "ReplayStartedEvent size must be 48 bytes");
static_assert(alignof(ReplayStartedEvent) == 8, "ReplayStartedEvent alignment must be 8 bytes");

struct alignas(8) ReplayFinishedEvent {
    EventHeader header;
    uint64_t end_sequence = 0;
};
static_assert(sizeof(ReplayFinishedEvent) == 48, "ReplayFinishedEvent size must be 48 bytes");
static_assert(alignof(ReplayFinishedEvent) == 8, "ReplayFinishedEvent alignment must be 8 bytes");

// Trait Specializations
template <> struct is_system_event<HeartbeatEvent> : std::true_type {};
template <> struct is_system_event<StartupEvent> : std::true_type {};
template <> struct is_system_event<ShutdownEvent> : std::true_type {};

template <> struct is_replay_event<ReplayStartedEvent> : std::true_type {};
template <> struct is_replay_event<ReplayFinishedEvent> : std::true_type {};

} // namespace fluxtrade
