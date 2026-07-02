add_library(storage INTERFACE)
target_include_directories(storage INTERFACE include)
target_link_libraries(storage INTERFACE common core matching)
# Note: libpqxx / hiredis dependencies will be integrated here once implementation begins.
