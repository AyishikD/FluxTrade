add_library(kernel STATIC
    src/dependency_resolver.cpp
    src/thread_manager.cpp
    src/health_monitor.cpp
    src/kernel.cpp
)

target_include_directories(kernel PUBLIC include)
target_link_libraries(kernel 
    PUBLIC 
        common 
        config
)
