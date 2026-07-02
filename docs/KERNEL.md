# FluxTrade Kernel Documentation

The **FluxTrade Kernel** acts as the engine bootloader, configuration mapping node, lifecycle manager, and thread pinning coordinator. By centralizing these orchestration concerns, we guarantee that hot-path execution modules (e.g., the Matching Engine) remain completely free of system orchestration logic.

---

## 1. System Orchestration Lifecycle

Subsystems within FluxTrade inherit from `IModule` and implement life-cycle transition triggers defined in `ILifecycle`.

### Module State Machine
Every module transitions through a state machine managed by the Kernel's coordinator:
```text
[Created] 
   │
   ▼ (initialize)
[Initializing] 
   │
   ▼ (configure)
[Configured] 
   │
   ▼ (start)
[Starting] ──> [Running] ──> (pause) ──> [Paused]
                  │                         │
                  ├──> [Failed]             └──> (resume) 
                  │
                  ▼ (stop)
[Stopping] ──> [Stopped] 
   │
   ▼ (shutdown)
[Shutdown]
```

---

## 2. Component Reference

### 2.1 Dependency Resolver (`DependencyResolver`)
Utilizes Kahn's topological sorting algorithm to resolve the execution dependency graph of all registered modules at boot time.
- **Validations**: Automatically rejects self-dependencies, duplicate modules, and circular references.
- **Ordering**: Returns an ordered list of `ModuleId`s for startup. During shutdown, the Kernel traverses the list in reverse order to ensure dependants are shutdown prior to their dependencies.

### 2.2 Thread Manager (`ThreadManager`)
Manages thread creation, system naming, and hardware affinity.
- **Core Pinning**: Configures the executing thread to bind exclusively to specified core IDs, reducing OS migration latency. Uses POSIX thread sets on Linux and Mach policies on macOS.
- **Naming**: Names threads inside the kernel (e.g., `matching_BTCUSDT`) for debugging diagnostics (visible in `htop`/`Instruments`).

### 2.3 Clock Abstraction (`Clock`)
Unified timing module. Exposes static methods retrieving nanoseconds or CPU clock cycles:
- **Assembly TSC Reads**: Executes inline assembler on ARM64 (`mrs cntvct_el0`) and x86_64 (`__rdtsc()`) to read the CPU cycle counter in single-digit nanoseconds.

### 2.4 Service Registry (`ServiceRegistry`)
Provides a centralized dependency injection container.
- Modules query the registry using standard C++ types (e.g., `registry->get_service<Clock>()`).
- Internally maps `std::type_index` to static void pointers, avoiding runtime RTTI or `dynamic_cast` downcasts.

---

## 3. Error Handling Policy
To ensure clean error boundaries:
- **Lifecycle transitions** return `expected<void, std::string>`.
- **System configuration** and **dependency checking** return `expected` structures.
- Exceptions are only thrown during unrecoverable startup failures (e.g., circular module dependencies or hardware pinning failures) to immediately halt the process.
