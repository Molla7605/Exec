add_executable(ExecExampleAsyncTask)
add_executable(Exec::Examples::AsyncTask ALIAS ExecExampleAsyncTask)

target_link_libraries(ExecExampleAsyncTask PUBLIC Exec)

target_sources(ExecExampleAsyncTask PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/async_task.cpp)