add_library(metrics INTERFACE)
target_include_directories(metrics INTERFACE include)
target_link_libraries(metrics INTERFACE common)
