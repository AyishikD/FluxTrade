add_library(events INTERFACE)
target_include_directories(events INTERFACE include)
target_link_libraries(events INTERFACE common kernel)
