cmake_minimum_required(VERSION 2.8)

set(PROJECT_NAME test)

add_executable(extlog-test extlog_test.c)
target_link_libraries(extlog-test cmocka cx)
add_test(extlog-test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/extlog-test)
