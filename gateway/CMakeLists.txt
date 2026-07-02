add_library(gateway STATIC
    src/gateway.cpp
)

target_include_directories(gateway PUBLIC
    include
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(gateway PUBLIC
    common
    domain
    kernel
)
