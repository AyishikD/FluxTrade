#pragma once

#include <kernel/module.hpp>
#include <kernel/service_registry.hpp>
#include <kernel/thread_manager.hpp>
#include <kernel/clock.hpp>
#include <kernel/health_monitor.hpp>
#include <kernel/event_bus.hpp>
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <expected>
#include <atomic>

namespace fluxtrade {

class Kernel {
public:
    Kernel();
    ~Kernel();

    Kernel(const Kernel&) = delete;
    Kernel& operator=(const Kernel&) = delete;

    // Registers a concrete module with the Kernel.
    template <typename T, typename... Args>
    expected<void, std::string> register_module(Args&&... args) {
        auto module = std::make_unique<T>(std::forward<Args>(args)...);
        ModuleId id = module->id();

        if (modules_.find(id) != modules_.end()) {
            return unexpected<std::string>("Module with ID " + std::to_string(static_cast<uint16_t>(id)) + " already registered");
        }

        modules_[id] = std::move(module);
        return {};
    }

    // Resolves dependencies, sets states, and initialises modules in topological order.
    expected<void, std::string> initialize();

    // Runs configuration on all sorted modules.
    expected<void, std::string> configure();

    // Spawns and starts all sorted modules.
    expected<void, std::string> start();

    // Stops all modules in reverse topological order.
    expected<void, std::string> stop() noexcept;

    // Shuts down all modules in reverse topological order.
    expected<void, std::string> shutdown() noexcept;

    // Blocks the calling thread, monitoring health and waiting for SIGINT/SIGTERM.
    void wait() noexcept;

    // Direct access to the shared service registry for registration/DI purposes.
    [[nodiscard]] ServiceRegistry& registry() noexcept { return registry_; }
    [[nodiscard]] const ServiceRegistry& registry() const noexcept { return registry_; }

    // Static callback hook for signal handlers
    static void handle_signal(int signal) noexcept;

private:
    void print_startup_graph() const noexcept;

    ServiceRegistry registry_;
    std::unordered_map<ModuleId, std::unique_ptr<IModule>> modules_;
    std::vector<ModuleId> startup_order_;

    // Core services owned by the Kernel
    ThreadManager thread_manager_;
    HealthMonitor health_monitor_;
    KernelEventBus event_bus_;

    static std::atomic<bool> shutdown_requested_;
};

} // namespace fluxtrade
