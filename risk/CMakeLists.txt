add_library(risk STATIC
    src/credit_manager.cpp
    src/margin_manager.cpp
    src/risk_engine.cpp
)

target_include_directories(risk PUBLIC 
    include
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(risk PUBLIC
    common
    domain
)
