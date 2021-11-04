project(Injectable CXX)

add_library(
        Injectable SHARED

        injectable.cpp

        cmd_queue.hpp
        utility.hpp

        v8_adapters/base_adapter.hpp
        v8_adapters/base_adapter.cpp

        v8_adapters/v8_adapter_v6.hpp
        v8_adapters/v8_adapter_v6.cpp
        v8_adapters/headers/6.0.286.52/source.cpp
)


target_link_libraries(Injectable PolyHook_2)
target_include_directories(Injectable SYSTEM PRIVATE ../PolyHook_2_0)
target_include_directories(Injectable SYSTEM PRIVATE ../PolyHook_2_0/zydis/include)
target_include_directories(Injectable SYSTEM PRIVATE ../PolyHook_2_0/zydis/dependencies/zycore/include)
target_include_directories(Injectable SYSTEM PRIVATE $<TARGET_FILE_DIR:Zydis>)
target_include_directories(Injectable SYSTEM PRIVATE $<TARGET_FILE_DIR:Zycore>)
