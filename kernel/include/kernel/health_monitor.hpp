#pragma once

#include <kernel/module.hpp>
#include <unordered_map>
#include <mutex>
#include <string>

namespace fluxtrade {

struct ModuleHealthInfo {
    ModuleId id;
    std::string name;
    ModuleState state = ModuleState::Created;
    HealthStatus health = HealthStatus::Healthy;
    uint64_t last_heartbeat_time = 0;
};

class HealthMonitor {
public:
    HealthMonitor() = default;
    ~HealthMonitor() = default;

    HealthMonitor(const HealthMonitor&) = delete;
    HealthMonitor& operator=(const HealthMonitor&) = delete;

    // Registers/updates a module heartbeat timestamp.
    void record_heartbeat(ModuleId module_id) noexcept;

    // Updates the state and health status of a module.
    void update_status(ModuleId module_id, ModuleState state, HealthStatus health) noexcept;

    // Checks the overall system health, returns true if degraded or unhealthy.
    [[nodiscard]] bool has_critical_failures() const noexcept;

    // Retrieves health details for all monitored modules.
    [[nodiscard]] std::vector<ModuleHealthInfo> get_health_report() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<ModuleId, ModuleHealthInfo> stats_;
};

} // namespace fluxtrade
