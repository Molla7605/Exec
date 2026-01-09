add_executable(ExecExampleErrorHandling)
add_executable(Exec::Examples::ErrorHandling ALIAS ExecExampleErrorHandling)

target_link_libraries(ExecExampleErrorHandling PUBLIC Exec)

target_sources(ExecExampleErrorHandling PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/error_handling.cpp)