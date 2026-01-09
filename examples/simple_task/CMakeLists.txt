add_executable(ExecExampleSimpleTask)
add_executable(Exec::Examples::SimpleTask ALIAS ExecExampleSimpleTask)

target_link_libraries(ExecExampleSimpleTask PUBLIC Exec)

target_sources(ExecExampleSimpleTask PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/simple_task.cpp)