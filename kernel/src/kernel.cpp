#include <kernel/kernel.hpp>
#include <kernel/dependency_resolver.hpp>
#include <iostream>
#include <csignal>
#include <iomanip>

namespace fluxtrade {

std::atomic<bool> Kernel::shutdown_requested_{false};

Kernel::Kernel() {
    // Register kernel core services into DI registry
    registry_.register_service(&thread_manager_);
    registry_.register_service(&health_monitor_);
    registry_.register_service(&event_bus_);

    // Register OS system interrupts
    std::signal(SIGINT, &Kernel::handle_signal);
    std::signal(SIGTERM, &Kernel::handle_signal);
    shutdown_requested_.store(false);
}

Kernel::~Kernel() {
    stop();
    shutdown();
}

void Kernel::handle_signal(int signal) noexcept {
    if (signal == SIGINT || signal == SIGTERM) {
        shutdown_requested_.store(true, std::memory_order_relaxed);
    }
}

expected<void, std::string> Kernel::initialize() {
    // Collect raw pointers for topological sort
    std::unordered_map<ModuleId, IModule*> module_ptrs;
    for (const auto& [id, module] : modules_) {
        module_ptrs[id] = module.get();
    }

    auto resolve_res = DependencyResolver::resolve(module_ptrs);
    if (!resolve_res) {
        return unexpected<std::string>("Dependency resolution failed: " + resolve_res.error());
    }

    startup_order_ = std::move(*resolve_res);

    for (ModuleId id : startup_order_) {
        auto& module = modules_[id];
        health_monitor_.update_status(id, ModuleState::Initializing, HealthStatus::Healthy);
        
        auto init_res = module->initialize();
        if (!init_res) {
            health_monitor_.update_status(id, ModuleState::Failed, HealthStatus::Unhealthy);
            return unexpected<std::string>("Module '" + std::string(module->name()) + "' failed to initialize: " + init_res.error());
        }
        health_monitor_.update_status(id, ModuleState::Configured, HealthStatus::Healthy);
    }

    return {};
}

expected<void, std::string> Kernel::configure() {
    for (ModuleId id : startup_order_) {
        auto& module = modules_[id];
        
        auto config_res = module->configure();
        if (!config_res) {
            health_monitor_.update_status(id, ModuleState::Failed, HealthStatus::Unhealthy);
            return unexpected<std::string>("Module '" + std::string(module->name()) + "' failed to configure: " + config_res.error());
        }
        health_monitor_.update_status(id, ModuleState::Configured, HealthStatus::Healthy);
    }
    return {};
}

expected<void, std::string> Kernel::start() {
    std::cout << "\nLoading modules..." << std::endl;
    for (ModuleId id : startup_order_) {
        auto& module = modules_[id];
        health_monitor_.update_status(id, ModuleState::Starting, HealthStatus::Healthy);

        auto start_res = module->start();
        if (!start_res) {
            health_monitor_.update_status(id, ModuleState::Failed, HealthStatus::Unhealthy);
            std::cout << std::left << std::setw(15) << module->name() << " FAILED" << std::endl;
            return unexpected<std::string>("Module '" + std::string(module->name()) + "' failed to start: " + start_res.error());
        }

        health_monitor_.update_status(id, ModuleState::Running, HealthStatus::Healthy);
        health_monitor_.record_heartbeat(id);
        
        std::cout << std::left << std::setw(15) << module->name() << " OK" << std::endl;
    }

    std::cout << "Startup complete." << std::endl;
    std::cout << startup_order_.size() << " modules initialized.\n" << std::endl;
    return {};
}

expected<void, std::string> Kernel::stop() noexcept {
    // Traverse in reverse order of startup
    for (auto it = startup_order_.rbegin(); it != startup_order_.rend(); ++it) {
        ModuleId id = *it;
        auto& module = modules_[id];
        if (module->state() == ModuleState::Running) {
            health_monitor_.update_status(id, ModuleState::Stopping, HealthStatus::Healthy);
            auto stop_res = module->stop();
            health_monitor_.update_status(id, ModuleState::Stopped, HealthStatus::Healthy);
        }
    }
    return {};
}

expected<void, std::string> Kernel::shutdown() noexcept {
    for (auto it = startup_order_.rbegin(); it != startup_order_.rend(); ++it) {
        ModuleId id = *it;
        auto& module = modules_[id];
        auto shut_res = module->shutdown();
        health_monitor_.update_status(id, ModuleState::Shutdown, HealthStatus::Healthy);
    }
    thread_manager_.join_all();
    return {};
}

void Kernel::wait() noexcept {
    while (!shutdown_requested_.load(std::memory_order_relaxed) && 
           !health_monitor_.has_critical_failures()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

} // namespace fluxtrade
