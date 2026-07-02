# ADR-006: Kernel Lifecycle & Dependency Sorting

## Context
When launching the exchange, modules must start in a precise order (e.g. `EventBus` and `Clock` must initialize before `Matching`, and `Matching` must initialize before `Network` listeners start). Shutting down must happen in the exact reverse order. Hardcoded sequence calls are prone to developer error, causing crashes on startup or shutdown.

## Decision
We enforce a **Dynamic Topological Lifecycle Sorting** system within the Kernel.
- Modules report their dependencies as a list of `ModuleId` enums.
- The Kernel runs Kahn's algorithm at startup to sort modules into a Directed Acyclic Graph (DAG).
- Shutdown operations are automatically executed in the exact reverse of the computed startup order.

## Alternatives Considered
1. **Hardcoded Arrays**: Specifying startup sequences manually in a configuration file or source array. Rejected because addition of new modules requires manual recalculation of dependencies, which is prone to regression.

## Consequences
- **Automatic Ordering**: Adding new modules requires no changes to the Kernel orchestrator.
- **Fail-Fast Startup**: Dependency circles, missing dependencies, and self-dependencies are caught at startup, causing a graceful termination before spawning threads.
- **Robust Shutdown**: Dependents are shut down before dependencies, preventing dangling references.

## Trade-offs
- **Complexity**: Requires graph-theoretic modules in the boot path.
- **Dynamic Checks**: Topological sort is computed at runtime on boot (a tiny one-time latency cost on startup).
