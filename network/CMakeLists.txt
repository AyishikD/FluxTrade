add_library(network INTERFACE)
target_include_directories(network INTERFACE include)
target_link_libraries(network INTERFACE common core)
# Note: Boost dependency will be fully integrated here once code execution begins.
