project(HeaderGenerator CXX)

add_executable(HeaderGenerator header_generator.cpp parser.cpp)

target_link_libraries(HeaderGenerator Dbghelp.lib)
