add_executable(simulator simulator.cpp)
target_link_libraries(simulator ipc common)
target_include_directories(simulator PRIVATE ${CMAKE_SOURCE_DIR}/include)
