#pragma once

#include <thread>
#include <system_error>

#ifdef __APPLE__
#include <mach/thread_act.h>
#include <mach/thread_policy.h>
#include <mach/mach_init.h>
#elif defined(__linux__)
#include <pthread.h>
#include <sched.h>
#endif

namespace fluxtrade {

inline bool pin_thread(uint32_t core_id) noexcept {
#ifdef __APPLE__
    thread_affinity_policy_data_t policy = { static_cast<integer_t>(core_id) };
    kern_return_t ret = thread_policy_set(
        mach_thread_self(),
        THREAD_AFFINITY_POLICY,
        reinterpret_cast<thread_policy_t>(&policy),
        THREAD_AFFINITY_POLICY_COUNT
    );
    return ret == KERN_SUCCESS;
#elif defined(__linux__)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_t current_thread = pthread_self();
    return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset) == 0;
#else
    (void)core_id;
    return false;
#endif
}

inline bool unbind() noexcept {
#ifdef __APPLE__
    thread_affinity_policy_data_t policy = { 0 };
    kern_return_t ret = thread_policy_set(
        mach_thread_self(),
        THREAD_AFFINITY_POLICY,
        reinterpret_cast<thread_policy_t>(&policy),
        THREAD_AFFINITY_POLICY_COUNT
    );
    return ret == KERN_SUCCESS;
#elif defined(__linux__)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (int i = 0; i < CPU_SETSIZE; ++i) {
        CPU_SET(i, &cpuset);
    }
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
#else
    return false;
#endif
}

inline int current_core() noexcept {
#if defined(__linux__)
    return sched_getcpu();
#else
    // macOS has no stable, direct user-space API to fetch current physical CPU core ID.
    return -1;
#endif
}

} // namespace fluxtrade
