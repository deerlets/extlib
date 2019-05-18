cmake_minimum_required(VERSION 2.8)

set(PROJECT_NAME tests)

add_executable(test-extlog test_extlog.c)
target_link_libraries(test-extlog cmocka cx)
add_test(test-extlog ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-extlog)

add_executable(test-extopt test_extopt.c)
target_link_libraries(test-extopt cmocka cx)
add_test(test-extopt ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-extopt)
