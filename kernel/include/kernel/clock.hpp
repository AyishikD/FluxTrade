#pragma once

#include <chrono>
#include <cstdint>

#if defined(__x86_64__) || defined(_M_X64)
#include <x86intrin.h>
#endif

namespace fluxtrade {

using Timestamp = uint64_t; // Nanoseconds since epoch

class Clock {
public:
    // Returns system time (wall clock) in nanoseconds since epoch
    static Timestamp now_system() noexcept {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    // Returns monotonic time in nanoseconds since epoch
    static Timestamp now_steady() noexcept {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }

    // Returns high resolution time in nanoseconds since epoch
    static Timestamp now_high_res() noexcept {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count();
    }

    // Returns raw CPU clock cycles (Time Stamp Counter)
    static uint64_t now_cycles() noexcept {
#if defined(__x86_64__) || defined(_M_X64)
        return __rdtsc();
#elif defined(__aarch64__)
        uint64_t val;
        // Read virtual counter register on ARM64
        asm volatile("mrs %0, cntvct_el0" : "=r" (val));
        return val;
#else
        // Fallback for other architectures
        return now_steady();
#endif
    }
};

} // namespace fluxtrade
