add_library(core INTERFACE)
target_include_directories(core INTERFACE include)
target_link_libraries(core INTERFACE common risk router)
