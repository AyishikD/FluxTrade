# ADR-004: Object Pool Memory Allocator

## Context
Standard C++ memory allocations via `new`/`delete` (which delegate to the system allocator `malloc`/`free`) are non-deterministic. They require locks to manage memory arenas, navigate free lists, and merge adjacent blocks. In multi-threaded systems, this introduces lock contention and memory fragmentation, causing significant latency spikes (jitter).

## Decision
We enforce a **Zero-Heap-Allocation Hot Path**. All dynamic objects generated during exchange execution (such as limit orders, trades, and event objects) are allocated from custom, preallocated single-threaded `ObjectPool` structures.

## Alternatives Considered
1. **System Allocator**: Standard `new`/`delete`. Rejected due to non-deterministic latency tail.
2. **tcmalloc / jemalloc**: High-performance system allocators. While much faster than standard libc allocators, they still feature synchronization overhead and periodic housekeeping cycles that introduce latency spikes.
3. **C++17 std::pmr (Polymorphic Memory Resources)**: Standard memory resource pools. Rejected to maintain strict C++20 standard library dependency-free compliance and retain fine-grained cache-line control over our custom pool storage.

## Consequences
- **Constant Time Allocation**: `ObjectPool::allocate()` and `ObjectPool::deallocate()` complete in $O(1)$ time (~3.6 ns).
- **Zero Fragmentation**: All elements are uniform in size and reside in a contiguous block of preallocated memory.
- **Improved L1/L2 Cache Locality**: Reusing recently deallocated memory blocks increases CPU cache hit rates.

## Trade-offs
- **Fixed Size Limit**: The exchange must configure pool capacities correctly at boot time. Running out of pool slots causes allocation failures (`std::bad_alloc`).
- **Memory Footprint**: System RAM is preallocated at startup, increasing memory footprint.
