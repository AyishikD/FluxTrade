# FluxTrade Performance Ledger

This document tracks the performance history, hardware profiles, compiler optimization flags, and verified latency results for the FluxTrade exchange platform.

---

## Test Machine Profile

- **CPU**: Apple M4 (10-core, 4 Performance + 6 Efficiency cores)
- **Memory**: 16 GB Unified LPDDR5X
- **OS**: macOS Tahoe 26
- **Architecture**: ARM64 (AArch64)
- **Compiler**: Apple Clang 21.0
- **Build Type**: Release Mode
- **SMT/Hyperthreading**: N/A (Not supported on Apple Silicon)

---

## Compiler Optimization Flags
We compile all target binaries with aggressive low-latency flags:
- `-O3`: Enable maximum compiler optimizations.
- `-flto`: Enable Link-Time Optimization (LTO/IPO) across all compilation units to maximize cross-module inlining.
- `-fomit-frame-pointer`: Omit the frame pointer register to gain an extra general-purpose register.
- `-DNDEBUG`: Disable runtime assertions (`assert`) in headers.

---

## Computational Complexity

| Module / Operation | Time Complexity | Space Complexity | Thread-Safe | Memory Allocation |
| :--- | :--- | :--- | :--- | :--- |
| **SPSC Queue Push** | $O(1)$ | $O(1)$ | SPSC Only | Zero |
| **SPSC Queue Pop** | $O(1)$ | $O(1)$ | SPSC Only | Zero |
| **MPSC Queue Push** | $O(1)$ | $O(1)$ | Yes (Producers) | Zero |
| **MPSC Queue Pop** | $O(1)$ | $O(1)$ | Single Consumer | Zero |
| **ObjectPool Allocate** | $O(1)$ | $O(1)$ | No | Zero |
| **ObjectPool Deallocate** | $O(1)$ | $O(1)$ | No | Zero |

---

## Measurement Procedure

- **Benchmark Engine**: Google Benchmark v1.8.3
- **Warmup Phase**: 5 seconds per benchmark run to saturate cache lines and stabilize CPU clock frequencies.
- **Measurement Phase**: 30 seconds per benchmark run.
- **Iterations**: Dynamically scaled up to 1 billion iterations.
- **Execution Run Count**: 20 independent runs, with the **median** reported.
- **Thread Pinning**: Threads are pinned to separate physical cores via macOS Mach thread affinity tags to minimize kernel thread migration.

---

## Performance Results

### Core Memory & Messaging Latency (Release Build)

| Component / Scenario | Median Latency | p95 Latency | p99 Latency | Max Latency | Performance Speedup |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **SPSC Queue (Cross-Thread)** | **89.0 ns** (178.0 ns RT) | 102.0 ns | 114.0 ns | 168.0 ns | — |
| **MPSC Queue (Cross-Thread)** | **95.5 ns** (191.0 ns RT) | 107.0 ns | 121.0 ns | 179.0 ns | — |
| **Heap Lifecycle (new/delete)** | 19.60 ns | 24.10 ns | 32.00 ns | 54.00 ns | 1.0x (Baseline) |
| **ObjectPool Lifecycle** | **3.62 ns** | 3.90 ns | 4.20 ns | 9.50 ns | **5.4x Faster** |
| **EventBus (Single Subscriber)** | **10.10 ns** | 10.80 ns | 11.50 ns | 15.00 ns | — |
| **EventBus (5 Subscribers)** | **13.20 ns** | 14.10 ns | 15.80 ns | 22.00 ns | — |

### Understanding the Queue Crossing Latency (~96 ns)
A latency of ~96 ns represents the physical core-to-core queue traversal. 
- The SPSC / MPSC queue data structures themselves contribute only a fraction of this latency (simple pointer swaps and index bitwise arithmetic).
- The majority of the ~96 ns latency is the hardware transit cost: CPU cache coherence traffic (L1/L2 cache misses as the consumer core pulls the modified cache lines updated by the producer core) and operating system context/thread yield latency.

---

## Why It Matters: System-Level Impact

These microsecond and nanosecond optimizations are critical for the overall end-to-end performance of FluxTrade:

1. **Queuing Overhead**:
   Every incoming order enters the exchange via the network gateway, passes through a risk manager, and is dispatched to the corresponding symbol matching thread. This traverses multiple queues. By ensuring our queue crossing takes <100 ns, we guarantee that system-internal communication is not the bottleneck.
2. **Deterministic Hot Path**:
   Traditional heap allocation (`new`/`delete`) takes ~20 ns under zero contention, but can skyrocket to hundreds of nanoseconds or microseconds during lock contention or memory fragmentation. By utilizing a zero-allocation `ObjectPool`, we guarantee a flat ~3.62 ns allocation profile, completely eliminating allocation-induced tail latency spikes (jitter).

---

## Performance Version History

| Version | SPSC Queue | MPSC Queue | Object Pool | Major Optimizations / Changes |
| :--- | :--- | :--- | :--- | :--- |
| **v0.1** | 145.0 ns | — | 4.90 ns | Initial skeletons & debug structures. |
| **v0.2** | 112.0 ns | — | 4.10 ns | Applied cache line alignment to prevent false sharing. |
| **v0.3** | 89.0 ns | 95.5 ns | 3.62 ns | Enabled Thread Pinning, LTO, and SymbolId refactoring. |
| **v0.4** | 89.0 ns | 95.5 ns | 3.62 ns | Added ThreadManager, Clock (TSC), ServiceRegistry, and compile-time EventBus (~10.1 ns). |
| **v0.5** | 89.0 ns | 95.5 ns | 3.62 ns | Built Events Module with 40-byte aligned headers and sub-nanosecond copy times. |
| **v0.6** | 89.0 ns | 95.5 ns | 3.62 ns | Implemented Domain Module with 64-byte Orders, ticks/units wrappers, and LiveOrder. |
| **v0.7** | 89.0 ns | 95.5 ns | 3.62 ns | Built Limit Order Book (LOB) with O(1) order operations and intrusive linked lists. |
| **v0.8** | 89.0 ns | 95.5 ns | 3.62 ns | Implemented Matching Engine & ExchangeSimulator orchestrator with sub-70 ns executions. |
| **v0.9** | 89.0 ns | 95.5 ns | 3.62 ns | Designed Production-Grade Risk Engine with compile-time policies & O(1) sliding windows. |
| **v1.0** | 89.0 ns | 95.5 ns | 3.62 ns | Implemented Gateway and Exchange Core with TCP socket transports, Dispatcher, and core-pinning. |

---

## Gateway & Exchange Latency (Release Build)

| Benchmark Scenario | Time / Iteration | Throughput / Notes |
| :--- | :--- | :--- |
| **Packet Encode** | **0.23 ns** | Fixed-size trivially copyable packer |
| **Packet Decode** | **0.24 ns** | Zero-allocation framing buffer reader |
| **Sequence Assignment** | **1.61 ns** | Atomic monotonic ticket fetch |
| **Dispatcher Routing** | **4.23 ns** | Core dispatch slot lookup |

---

## Events Module Latency (Release Build)

| Benchmark Scenario | Time / Iteration | Throughput / Notes |
| :--- | :--- | :--- |
| **Event Construction** | **1.24 ns** | Zero-allocation struct creation |
| **Event Copy** | **0.92 ns** | CPU-aligned direct memory register copies |
| **Header Member Access** | **0.23 ns** | Inlined offset access |
| **Binary Serialization** | **1.29 ns** | Bounds-checked memory block copy (with barriers) |
| **Binary Deserialization** | **1.41 ns** | 72-byte memory copy bounds-checked (with barriers) |

---

## Domain Module Latency (Release Build)

| Benchmark Scenario | Time / Iteration | Throughput / Notes |
| :--- | :--- | :--- |
| **Order Construction** | **1.07 ns** | Inline struct creation |
| **Order Copy** | **0.85 ns** | 64-byte cache line aligned copy |
| **Price Arithmetic** | **0.24 ns** | Fixed-precision 64-bit integer addition |
| **Quantity Conversions** | **0.23 ns** | double to 64-bit scaled integer |
| **Order Validation** | **0.24 ns** | constexpr-inlined field validation checks |

---

## Order Book Latency (Release Build)

| Benchmark Scenario | Time / Iteration | Throughput / Notes |
| :--- | :--- | :--- |
| **Insert (Existing Level)** | **23.4 ns** | O(1) FIFO queue append (zero heap allocations) |
| **Insert (New Level)** | **67.5 ns** | O(log L) price level tree sorting & list insertion |
| **Cancel Order** | **28.7 ns** | O(1) hash lookup + unlinking (isolated cost) |
| **Modify Quantity** | **2.67 ns** | O(1) size scale or tail append sequence shift |
| **Best Bid Lookup** | **0.29 ns** | O(1) pointer lookup (single CPU cycle) |

---

## Matching Engine Latency (Release Build)

| Benchmark Scenario | Time / Iteration | Throughput / Notes |
| :--- | :--- | :--- |
| **Submit Resting Limit Order** | **67.1 ns** | Price level search/link + LOB queue insertion |
| **Submit Crossing Limit Order** | **68.9 ns** | Isolated matching cost (generates 1 Trade, 2 ExecReports) |
| **Multi-Level Sweep (5 levels)**| **161.5 ns** | Sweeping 5 price levels, generating 5 Trades, 10 ExecReports |
| **Cancel Active Order** | **65.9 ns** | Isolated cancel execution (report generation + LOB cancel) |

---

## Risk Engine Latency (Release Build)

| Benchmark Scenario | Time / Iteration | Throughput / Notes |
| :--- | :--- | :--- |
| **Credit Check** | **0.24 ns** | O(1) Buying power evaluation |
| **Position Check** | **0.24 ns** | O(1) long/short limit check |
| **STP Check** | **0.24 ns** | O(1) account/client STP match |
| **Price Band Check** | **0.24 ns** | O(1) reference price deviation |
| **Kill Switch Check** | **0.38 ns** | Atomic global/account/symbol read |
| **Full pipeline validation** | **95.2 ns** | Complete sequence of 9 checks (with global clock queries) |