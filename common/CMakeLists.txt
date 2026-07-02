add_library(common INTERFACE)
target_include_directories(common INTERFACE include)
target_link_libraries(common INTERFACE Threads::Threads flux_compile_options)
