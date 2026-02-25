add_executable(ExecExampleThreadPool)
add_executable(Exec::Examples::ThreadPool ALIAS ExecExampleThreadPool)

target_link_libraries(ExecExampleThreadPool PUBLIC Exec)

target_sources(ExecExampleThreadPool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool.cpp)