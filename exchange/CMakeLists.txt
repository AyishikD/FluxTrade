add_library(exchange STATIC
    src/exchange.cpp
)

target_include_directories(exchange PUBLIC
    include
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(exchange PUBLIC
    common
    domain
    kernel
    gateway
    risk
    matching
    persistence
)
