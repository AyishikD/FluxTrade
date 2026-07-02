#include <kernel/health_monitor.hpp>
#include <kernel/clock.hpp>

namespace fluxtrade {

void HealthMonitor::record_heartbeat(ModuleId module_id) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& info = stats_[module_id];
    info.id = module_id;
    info.last_heartbeat_time = Clock::now_steady();
}

void HealthMonitor::update_status(ModuleId module_id, ModuleState state, HealthStatus health) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& info = stats_[module_id];
    info.id = module_id;
    info.state = state;
    info.health = health;
}

bool HealthMonitor::has_critical_failures() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [id, info] : stats_) {
        if (info.state == ModuleState::Failed || info.health == HealthStatus::Unhealthy) {
            return true;
        }
    }
    return false;
}

std::vector<ModuleHealthInfo> HealthMonitor::get_health_report() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ModuleHealthInfo> report;
    report.reserve(stats_.size());
    for (const auto& [id, info] : stats_) {
        report.push_back(info);
    }
    return report;
}

} // namespace fluxtrade
