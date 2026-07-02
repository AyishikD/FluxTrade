include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
# Prevent overriding compiler settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(flux_tests
    common/ring_buffer_tests.cpp
    common/object_pool_tests.cpp
    kernel/dependency_resolver_tests.cpp
    kernel/thread_manager_tests.cpp
    kernel/event_bus_tests.cpp
    kernel/kernel_tests.cpp
    events/event_tests.cpp
    domain/domain_tests.cpp
    matching/order_book_tests.cpp
    matching/matching_engine_tests.cpp
)

target_link_libraries(flux_tests 
    PRIVATE 
        gtest_main 
        common
        kernel
        events
        domain
        matching
        flux_compile_options
)

# Enable testing under CTest
include(GoogleTest)
gtest_discover_tests(flux_tests)
