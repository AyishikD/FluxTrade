add_library(domain INTERFACE)
target_include_directories(domain INTERFACE include)
target_link_libraries(domain INTERFACE common)
