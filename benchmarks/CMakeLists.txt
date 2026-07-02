include(FetchContent)

FetchContent_Declare(
    googlebenchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.tar.gz
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(flux_benchmarks
    queue/ring_buffer_benchmark.cpp
)

target_link_libraries(flux_benchmarks
    PRIVATE
        benchmark::benchmark
        common
        flux_compile_options
)

add_executable(event_benchmarks
    events/event_benchmark.cpp
)

target_link_libraries(event_benchmarks
    PRIVATE
        benchmark::benchmark
        common
        events
        flux_compile_options
)

add_executable(domain_benchmarks
    domain/domain_benchmark.cpp
)

target_link_libraries(domain_benchmarks
    PRIVATE
        benchmark::benchmark
        common
        domain
        flux_compile_options
)

add_executable(order_book_benchmarks
    matching/order_book_benchmark.cpp
)

target_link_libraries(order_book_benchmarks
    PRIVATE
        benchmark::benchmark
        common
        domain
        matching
        flux_compile_options
)

add_executable(matching_engine_benchmarks
    matching/matching_engine_benchmark.cpp
)

target_link_libraries(matching_engine_benchmarks
    PRIVATE
        benchmark::benchmark
        common
        domain
        matching
        flux_compile_options
)
