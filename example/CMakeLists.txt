project(FuelExample CXX)

add_executable(FuelExample fuel_example.cpp)

target_link_libraries(FuelExample FuelInjector)
