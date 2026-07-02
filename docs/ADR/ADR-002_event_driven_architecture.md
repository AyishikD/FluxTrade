# ADR-002: Event-Driven Architecture

## Context
A trading platform consists of many disparate subsystems: network ingestion gateways, pre-trade risk managers, order routers, matching engines, write-ahead loggers (WAL), database persistent layers, and dashboard aggregators. Direct coupling of these components leads to brittle code, circular dependencies, and high latency.

## Decision
FluxTrade adopts an **Event-Driven Architecture (EDA)**. Components communicate solely by publishing and subscribing to structured, binary event frames. Every event contains a standard `EventHeader` (mapping ID, sequence number, timestamp, and source module).

## Alternatives Considered
1. **Direct Synchronous Calls**: Gateways invoke the matching engine directly, which in turn calls the database and network writers. Rejected because a slow database write or network socket stall would block the entire matching execution path.
2. **Actor Model**: Utilizing actor frameworks. Rejected due to runtime library overhead and allocation jitter.

## Consequences
- **Decoupled Architecture**: The matching engine only consumes order events and emits execution events. It knows nothing about databases, network protocol frames, or Prometheus metrics.
- **Asynchronous Persistence**: Direct-I/O WAL writing and PostgreSQL updates are handled asynchronously by subscribing to trade event streams, separating persistence from matching.
- **Replayability**: Storing events sequentially allows full system reconstruction and deterministic backtesting.

## Trade-offs
- **Complexity**: Debugging asynchronous event pipelines requires tracing unique sequence IDs.
- **Queue Overhead**: Every transition across module boundaries introduces queue transit latency (~90-95 ns), which must be minimized.
