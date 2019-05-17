cmake_minimum_required(VERSION 2.8)

set(PROJECT_NAME test)

add_executable(extlog-test extlog_test.c)
target_link_libraries(extlog-test cmocka cx)
add_test(extlog-test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/extlog-test)

add_executable(extopt-test extopt_test.c)
target_link_libraries(extopt-test cmocka cx)
add_test(extopt-test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/extopt-test)
