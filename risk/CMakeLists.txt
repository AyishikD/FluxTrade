add_library(risk INTERFACE)
target_include_directories(risk INTERFACE include)
target_link_libraries(risk INTERFACE common)
