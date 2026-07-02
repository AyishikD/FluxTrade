#pragma once

#include <vector>
#include <functional>
#include <tuple>
#include <shared_mutex>
#include <utility>

namespace fluxtrade {

// Bounded Compile-Time Type-Safe EventBus
template <typename... Events>
class EventBus {
public:
    EventBus() = default;
    ~EventBus() = default;

    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    EventBus(EventBus&&) = delete;
    EventBus& operator=(EventBus&&) = delete;

    template <typename EventType>
    using Callback = std::function<void(const EventType&)>;

    // Subscribe to a specific compile-time registered event type.
    template <typename EventType>
    void subscribe(Callback<EventType> cb) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto& list = std::get<std::vector<Callback<EventType>>>(subscribers_);
        list.push_back(std::move(cb));
    }

    // Publish an event. Delivery is synchronous, lock-free on reads (shared lock), and zero-allocation.
    template <typename EventType>
    void publish(const EventType& event) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        const auto& list = std::get<std::vector<Callback<EventType>>>(subscribers_);
        for (const auto& cb : list) {
            cb(event);
        }
    }

private:
    mutable std::shared_mutex mutex_;
    std::tuple<std::vector<Callback<Events>>...> subscribers_;
};

// Core system events for Kernel orchestrations
struct SystemHeartbeatEvent {
    uint64_t timestamp;
};

struct SystemShutdownEvent {
    uint64_t timestamp;
};

// Default Kernel EventBus instance type
using KernelEventBus = EventBus<SystemHeartbeatEvent, SystemShutdownEvent>;

} // namespace fluxtrade
