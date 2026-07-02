#pragma once

#include <string_view>
#include <vector>
#include <string>
#include <common/expected.hpp>

namespace fluxtrade {

enum class ModuleId : uint16_t {
    Kernel,
    Config,
    Clock,
    Logger,
    EventBus,
    Scheduler,
    ThreadManager,
    HealthMonitor,
    Risk,
    OrderBook,
    Matching,
    Router,
    Network,
    Storage,
    Simulator,
    Replay
};

enum class ModuleState : uint8_t {
    Created,
    Initializing,
    Configured,
    Starting,
    Running,
    Paused,
    Stopping,
    Stopped,
    Shutdown,
    Failed
};

enum class HealthStatus : uint8_t {
    Healthy,
    Degraded,
    Unhealthy
};

class ILifecycle {
public:
    virtual ~ILifecycle() = default;

    virtual expected<void, std::string> initialize() = 0;
    virtual expected<void, std::string> configure() = 0;
    virtual expected<void, std::string> start() = 0;
    virtual expected<void, std::string> pause() = 0;
    virtual expected<void, std::string> resume() = 0;
    virtual expected<void, std::string> stop() = 0;
    virtual expected<void, std::string> shutdown() = 0;
};

class IHealthCheck {
public:
    virtual ~IHealthCheck() = default;
    virtual HealthStatus check_health() = 0;
};

class IModule : public ILifecycle, public IHealthCheck {
public:
    virtual ~IModule() = default;

    [[nodiscard]] virtual ModuleId id() const noexcept = 0;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual std::vector<ModuleId> dependencies() const = 0;
    [[nodiscard]] virtual ModuleState state() const noexcept = 0;
};

} // namespace fluxtrade
