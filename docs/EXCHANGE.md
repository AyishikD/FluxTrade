# FluxTrade Exchange Core Orchestrator

This document details the startup configurations, threading models, and execution coordinators of the **Exchange Core** module.

---

## 1. Threading Topology

The Exchange Core separates connections from matching execution by allocating dedicated, core-pinned execution loop threads per symbol:

```text
                  ┌──────────────────────────────┐
                  │        Gateway Thread        │
                  └──────────────┬───────────────┘
                                 │
                            Dispatcher
                                 │
          ┌──────────────────────┼──────────────────────┐
          ▼                      ▼                      ▼
  Engine Thread (AAPL)   Engine Thread (MSFT)   Engine Thread (BTC)
  ├── inline Risk check  ├── inline Risk check  ├── inline Risk check
  └── Matching Engine    └── Matching Engine    └── Matching Engine
```

### Thread Topology Benefits
1.  **Zero Locking**: Since each symbol engine is isolated inside its own thread, there is no lock contention when matching orders on different symbols.
2.  **Minimized Queue Hops**: Placing the Risk Engine validation check inline on the symbol engine thread avoids an extra cross-thread queue hop, reducing latency.

---

## 2. Startup and Shutdown Pipeline

### Startup Sequence
- Initialize `Exchange` orchestrator injecting `ITransport`.
- Register symbols (e.g. `add_symbol(symbol_id, core_id)`).
- Starts all `EngineManager` matching threads.
- Starts `response_thread_` loop.
- Opens the TCP listener gateway socket and launches the poller thread.

### Graceful Shutdown Sequence
- Triggers `stop()` setting the atomic thread termination flags.
- Joins all symbol thread loops and gateway pollers.
- Closes the gateway TCP sockets.
