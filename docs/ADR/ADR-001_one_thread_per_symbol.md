# ADR-001: One Thread Per Symbol

## Context
High-performance matching engines must handle thousands of orders per second with sub-microsecond latency. Traditional lock-based multi-threaded architectures (where multiple worker threads process orders on the same order book using mutexes or read-write locks) experience extreme thread contention, kernel locks, cache-line bouncing, and non-deterministic execution times.

## Decision
We enforce a strict **One Thread Per Symbol** concurrency model. Each active symbol (e.g., AAPL, MSFT, BTCUSDT) is owned by a single, dedicated execution thread pinned to a specific CPU core. That thread is the sole writer and reader of its associated Limit Order Book.

## Alternatives Considered
1. **Global Lock / Single Threaded Exchange**: Extremely simple to implement, but fails to utilize multi-core server processors, capping execution capacity.
2. **Fine-Grained Locking / Concurrent Order Book**: Multiple threads match orders concurrently on the same book using locks. Rejected due to synchronization overhead, kernel scheduling jitter, and tail-latency spikes.

## Consequences
- **Zero Locks**: The matching loop runs completely lock-free without standard atomic synchronization on the book.
- **Cache Locality**: Order book memory stays resident in the L1/L2 data cache of the pinned CPU core.
- **Sequential Ingestion**: Orders are processed sequentially in exact arrival order per symbol, ensuring strict price-time priority correctness.

## Trade-offs
- **Routing Overhead**: Requires a dedicated gateway/router layer to dispatch orders to symbol threads using lock-free queues.
- **Hardware Bound**: The maximum throughput of a single symbol is bound by the single-core speed of the host CPU.
