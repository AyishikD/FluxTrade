#include <kernel/thread_manager.hpp>
#include <common/thread_pinning.hpp>

#ifdef __APPLE__
#include <pthread.h>
#elif defined(__linux__)
#include <pthread.h>
#endif

namespace fluxtrade {

ThreadManager::~ThreadManager() {
    join_all();
}

void ThreadManager::join_all() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    threads_.clear();

    for (auto& [name, stats] : workers_) {
        stats.is_active = false;
    }
}

std::vector<WorkerStats> ThreadManager::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<WorkerStats> stats;
    stats.reserve(workers_.size());
    for (const auto& [name, s] : workers_) {
        stats.push_back(s);
    }
    return stats;
}

void ThreadManager::set_current_thread_name(const std::string& name) noexcept {
#ifdef __APPLE__
    // macOS only sets current thread's name
    pthread_setname_np(name.c_str());
#elif defined(__linux__)
    pthread_setname_np(pthread_self(), name.c_str());
#endif
}

void ThreadManager::pin_current_thread_to_core(int32_t core_id) noexcept {
    pin_thread(static_cast<uint32_t>(core_id));
}

} // namespace fluxtrade
