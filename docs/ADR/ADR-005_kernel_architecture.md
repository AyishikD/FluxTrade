# ADR-005: Kernel Architecture

## Context
A production-grade exchange requires system-level orchestration: configuration loading, dependency resolution, signal handling, CPU thread affinity configuration, log routing, and component lifecycles. Without a dedicated architecture, modules become tightly coupled, creating "God Objects" or spaghetti initialization code.

## Decision
We implement a centralized **FluxTrade Kernel**. The Kernel acts as the platform's bootloader and runtime coordinator. Modules register with the Kernel, declare their dependency requirements using compile-time `ModuleId` lists, and retrieve shared resources via a type-safe `ServiceRegistry`.

## Alternatives Considered
1. **Ad-Hoc Singleton Managers**: Managing systems via global static Singletons (`ConfigManager::getInstance()`, `LogManager::getInstance()`). Rejected because it creates hidden dependencies, prevents unit testing, and risks undefined initialization orders.
2. **Standard IoC Frameworks**: Utilizing heavy third-party dependency injection libraries. Rejected to prevent code bloat, RTTI compilation flags, and slow startup times.

## Consequences
- **Loose Coupling**: Modules do not know about other concrete modules; they only request interfaces (e.g., querying `ILogger` from the `ServiceRegistry`).
- **Testability**: Individual modules can be isolated and mock-tested by injecting custom dependencies into the registry.
- **Unified Controls**: Process signals (SIGINT, SIGTERM), health check reports, and watchdog loops are managed globally by the Kernel.

## Trade-offs
- **Registration Overhead**: Every new subsystem must implement the `IModule` interface and register with the Kernel.
- **Type Index Resolution**: Querying services from the registry incurs a slight hash lookup cost, though this is restricted to startup initialization.
