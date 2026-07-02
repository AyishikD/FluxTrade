add_library(router INTERFACE)
target_include_directories(router INTERFACE include)
target_link_libraries(router INTERFACE common matching)
