add_library(matching STATIC
    order_book/order_book.cpp
    src/trade_builder.cpp
    src/matching_engine.cpp
)

target_include_directories(matching PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(matching PUBLIC
    common
    domain
)
