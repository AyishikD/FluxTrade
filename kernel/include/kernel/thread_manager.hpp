#pragma once

#include <thread>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <future>
#include <common/expected.hpp>

namespace fluxtrade {

struct WorkerStats {
    std::string name;
    std::thread::id thread_id;
    int32_t core_id = -1;
    bool is_active = false;
};

class ThreadManager {
public:
    ThreadManager() = default;
    ~ThreadManager();

    ThreadManager(const ThreadManager&) = delete;
    ThreadManager& operator=(const ThreadManager&) = delete;
    ThreadManager(ThreadManager&&) = delete;
    ThreadManager& operator=(ThreadManager&&) = delete;

    // Spawns a thread, sets its CPU core affinity, assigns a name, and tracks its lifetime.
    template <typename Function, typename... Args>
    expected<std::thread::id, std::string> spawn_worker(
        const std::string& name,
        int32_t core_id,
        Function&& func,
        Args&&... args
    ) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Verify name uniqueness
        if (workers_.find(name) != workers_.end()) {
            return unexpected<std::string>("Worker with name '" + name + "' already exists");
        }

        std::promise<std::thread::id> tid_promise;
        auto tid_future = tid_promise.get_future();

        std::thread t([name, core_id, &tid_promise, 
                       f = std::forward<Function>(func), 
                       args_tuple = std::make_tuple(std::forward<Args>(args)...)]() mutable {
            // Set thread name (OS-specific)
            set_current_thread_name(name);

            // Set core affinity if requested (>= 0)
            if (core_id >= 0) {
                // Import pin_thread utility from common
                pin_current_thread_to_core(core_id);
            }

            // Expose the thread ID to caller before running the payload
            tid_promise.set_value(std::this_thread::get_id());

            // Run payload
            std::apply(std::move(f), std::move(args_tuple));
        });

        std::thread::id tid = tid_future.get();
        
        WorkerStats stats{
            .name = name,
            .thread_id = tid,
            .core_id = core_id,
            .is_active = true
        };

        threads_.push_back(std::move(t));
        workers_[name] = std::move(stats);

        return tid;
    }

    // Joins all spawned worker threads gracefully.
    void join_all() noexcept;

    // Retrieves execution stats for all registered workers.
    [[nodiscard]] std::vector<WorkerStats> get_stats() const;

private:
    static void set_current_thread_name(const std::string& name) noexcept;
    static void pin_current_thread_to_core(int32_t core_id) noexcept;

    mutable std::mutex mutex_;
    std::vector<std::thread> threads_;
    std::unordered_map<std::string, WorkerStats> workers_;
};

} // namespace fluxtrade
