cmake_minimum_required(VERSION 3.12)

add_executable(test-extlog test_extlog.c)
target_link_libraries(test-extlog cmocka cx)
add_test(test-extlog ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-extlog)

add_executable(test-extopt test_extopt.c)
target_link_libraries(test-extopt cmocka cx)
add_test(test-extopt ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-extopt)

add_executable(test-task test_task.c)
target_link_libraries(test-task cmocka cx)
add_test(test-task ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-task)

add_library(mock_module SHARED mock_module.c)
set_target_properties(mock_module PROPERTIES PREFIX "")
set_target_properties(mock_module PROPERTIES SUFFIX ".zo")

add_executable(test-module test_module.c)
target_link_libraries(test-module cmocka cx)
add_test(test-module ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-module)

add_executable(test-timer test_timer.c)
target_link_libraries(test-timer cmocka cx)
add_test(test-timer ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-timer)

add_executable(test-exterr test_exterr.c)
target_link_libraries(test-exterr cmocka cx)
add_test(test-exterr ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-exterr)
