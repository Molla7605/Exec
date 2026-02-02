cmake_minimum_required(VERSION 3.28)

project(Exec)

option(EXEC_BUILD_EXAMPLES "Build examples" ON)

add_library(Exec INTERFACE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(Exec INTERFACE cxx_std_23)

if (EXEC_BUILD_EXAMPLES)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples)
endif ()