# ADR-003: Lock-Free Queues

## Context
When routing events between different threads (e.g. from network gateway threads to risk check threads, or router threads to symbol matching threads), we need a thread-safe message transport. Standard queues (`std::queue` protected by a `std::mutex` and `std::condition_variable`) cause thread blocking, kernel context switching, and unpredictable latency tails.

## Decision
We implement and enforce custom **Lock-Free Bounded Queues**:
- **SPSCQueue**: Single-Producer Single-Consumer queue for point-to-point thread hops.
- **MPSCQueue**: Multi-Producer Single-Consumer queue (Dmitry Vyukov adaptation) for merging gateways into core sequencers.
Both queues are cache-aligned to prevent false sharing.

## Alternatives Considered
1. **Lock-Based Bounded Queues**: Rejected due to mutex locking latency (typically 50 ns to several microseconds under contention) and kernel scheduler wakeups.
2. **Dynamic Queues (e.g. lock-free linked lists)**: Rejected because dynamically allocating memory nodes during queue pushes causes heap allocator contention.

## Consequences
- **Sub-100ns Latency**: Queue crossings take only ~89-95 ns on modern CPUs (including cache coherency updates).
- **Predictable Performance**: Queue operations are wait-free and bounded, eliminating latency spikes.
- **Zero Allocations**: Ring buffers are pre-sized and pre-allocated during initialization.

## Trade-offs
- **Fixed Capacity**: Bounded queues can fill up under extreme load. The producer must yield, spin, or reject if the queue is full.
- **CPU Spinning**: To achieve maximum throughput, reading threads poll (spin) on the queues, increasing CPU utilization.
