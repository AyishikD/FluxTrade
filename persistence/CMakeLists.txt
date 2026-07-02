add_library(persistence INTERFACE)

target_include_directories(persistence INTERFACE
    include
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(persistence INTERFACE
    common
    domain
    events
)
