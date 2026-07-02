# FluxTrade

**FluxTrade** is a high-performance, deterministic electronic exchange simulator written in Modern C++20. Designed for sub-microsecond latency, the platform employs lock-free ring buffers, preallocated object pools, intrusive data structures, and standard L1/L2 cache-line optimization to sustain millions of messages per second.

---

## 🚀 Key Performance Indicators (LTO Release Build)

| Module | Operation | Time / Iteration | CPU Cycles (Est. at 4 GHz) |
| :--- | :--- | :--- | :--- |
| **Domain** | Price Arithmetic | **0.24 ns** | ~1.0 Cycle |
| **Domain** | Order Copy (64 bytes) | **0.85 ns** | ~3.4 Cycles |
| **Order Book** | Best Bid/Ask Pointer Lookup | **0.29 ns** | ~1.1 Cycles |
| **Order Book** | Order Insertion (Existing Level) | **23.4 ns** | ~93 Cycles |
| **Order Book** | Order Cancellation (Isolated) | **28.7 ns** | ~114 Cycles |
| **Matching Engine** | Crossing Match (1 Trade, 2 Reports) | **68.9 ns** | ~275 Cycles |
| **Matching Engine** | Multi-Level Sweep (5 levels, 5 Trades) | **161.5 ns** | ~646 Cycles |

---

## 🏗️ Architecture Layout

```text
       Inbound Gateway
             │
             ▼
      Risk Manager (Margin / Credit limits check)
             │
             ▼
    Deterministic Replay Log
             │
             ▼
       Matching Engine (Orchestrator) ─── [Preallocated ExecutionContext]
             │
      ┌──────┴──────┐
      ▼             ▼
  BidBook        AskBook
    (O(1) Intrusive doubly linked queues of LiveOrders)
```

### Core Subsystems

*   **Common Primitives**: Lock-free SPSC and MPSC queues, cache-aligned memory barriers, and custom, zero-overhead `expected<T, E>` variants.
*   **Kernel Lifecycle Manager**: Kahn's topological sorted dependency graph to orchestrate deterministic startup/shutdown routines across all modules.
*   **Events Dispatcher**: Packed, trivially copyable 40-byte EventHeader with bounds-checked, zero-allocation binary serialization.
*   **Domain Module**: Fixed-point price ($10^8$ ticks scale) and quantity ($10^6$ units scale) value objects, and a 64-byte client `Order` structure fitting perfectly inside a single L1 cache line.
*   **Limit Order Book (LOB)**: Decoupled `BidBook` (descending) and `AskBook` (ascending) that abstract level indexing, maintaining Price-Time FIFO order via intrusive linked lists.
*   **Matching Engine**: A stateless execution core matching crosses in under 70 ns. Utilizes a preallocated `ExecutionContext` to ensure zero runtime heap allocations.
*   **Exchange Simulator**: Symbol orchestrator mapping token identifiers to isolated, deterministic matching engine cores.

---

## 🛠️ Build and Compilation

### Prerequisites
*   CMake (3.20 or higher)
*   A C++20 compliant compiler (GCC 11+, Clang 13+, or Apple Clang 14+)

### Compilation (Release Build with Link-Time Optimization)
```bash
# Generate CMake build target
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build all libraries, tests, and benchmarks
cmake --build build --config Release
```

### Running Unit Tests
```bash
cd build
ctest --output-on-failure
```

### Running Micro-Benchmarks
To run the latency suite (requires Google Benchmark):
```bash
# Run Order Book Micro-benchmarks
./build/benchmarks/order_book_benchmarks

# Run Matching Engine Micro-benchmarks
./build/benchmarks/matching_engine_benchmarks
```

---

## 🎯 Design Philosophy

1.  **Zero Heap Allocation**: Memory blocks for resting orders and price levels are recycled through preallocated `ObjectPool` blocks, eliminating malloc/free delays on the critical matching path.
2.  **No RTTI, Virtual Dispatch, or Exceptions**: All polymorphism and bindings are resolved at compile-time via C++20 Concepts and templates.
3.  **Strict State Segregation**: The matching core drives executions, while storage remains encapsulated inside the order books.
4.  **Deterministic Execution**: Ordering relies solely on sequence numbers assigned before entering the engine, ensuring that replaying the same log produces identical states.
