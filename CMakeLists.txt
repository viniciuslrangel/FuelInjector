cmake_minimum_required(VERSION 3.14)
project(FuelInjector CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHa")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DFUEL_DEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DFUEL_DEBUG")

option(FUEL_EXAMPLE_PROJECT "" FALSE)

add_library(
        FuelInjector
        fuel_injector.cpp fuel_injector.hpp
        injectable_g.cpp
)
add_executable(
        Embed
        embed/embed.cpp
)

target_include_directories(FuelInjector PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        "$<INSTALL_INTERFACE:include>")


add_subdirectory(injectable)
add_subdirectory(header_generator)

IF (FUEL_EXAMPLE_PROJECT)
    add_subdirectory(example)
ENDIF ()

add_custom_command(
        OUTPUT injectable_g.cpp
        COMMAND $<TARGET_FILE:Embed> $<TARGET_FILE:Injectable> "injectable_g.cpp"
        DEPENDS Embed Injectable
        USES_TERMINAL
)

set(POLYHOOK_DLL OFF CACHE BOOL "")
set(POLYHOOK_DISASM_CAPSTONE OFF CACHE BOOL "")
set(POLYHOOK_FEATURE_INLINENTD OFF CACHE BOOL "")
add_subdirectory(PolyHook_2_0)
set_target_properties(PolyHook_2 PROPERTIES COMPILE_FLAGS "-w")
