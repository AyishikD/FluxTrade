cmake_minimum_required(VERSION 3.20)
project(FluxTrade
        VERSION 0.1.0
        LANGUAGES CXX)

# Export compile commands for IDE language servers (clangd/VSCode)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enforce C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimize for low-latency execution (O3, native architecture optimizations)
add_library(flux_compile_options INTERFACE)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(flux_compile_options INTERFACE 
        -O3 
        -march=native 
        -Wall 
        -Wextra 
        -Wpedantic 
        -Werror 
        -Wno-unused-parameter
        -fomit-frame-pointer
    )
    # Enable position independent code for plugins
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Enable Link-Time Optimization (LTO / IPO) for maximum performance
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
if(ipo_supported)
    set_target_properties(flux_compile_options PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()


# Thread support
find_package(Threads REQUIRED)

# Include directories
include_directories(
    common/include
    config/include
    events/include
    kernel/include
    core/include
    risk/include
    orderbook/include
    matching/include
    router/include
    execution/include
    marketdata/include
    scheduler/include
    metrics/include
    monitoring/include
    network/include
    storage/include
)

# Project Modules
add_subdirectory(common)
add_subdirectory(events)
add_subdirectory(domain)
add_subdirectory(config)
add_subdirectory(kernel)
add_subdirectory(core)
add_subdirectory(risk)
add_subdirectory(matching)
add_subdirectory(gateway)
add_subdirectory(exchange)
add_subdirectory(router)
add_subdirectory(execution)
add_subdirectory(marketdata)
add_subdirectory(scheduler)
add_subdirectory(metrics)
add_subdirectory(monitoring)
add_subdirectory(network)
add_subdirectory(storage)

# Testing & Benchmarking Setup (Enabled by default, can be toggled)
option(BUILD_TESTING "Build the test suite" ON)
option(BUILD_BENCHMARKS "Build micro-benchmarks" ON)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Simulators & Tools
add_subdirectory(simulator)
add_subdirectory(replay)
