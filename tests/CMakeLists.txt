cmake_minimum_required(VERSION 3.12)

add_executable(test-errx test_errx.c)
target_link_libraries(test-errx cmocka cx)
add_test(test-errx ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-errx)

add_executable(test-logx test_logx.c)
target_link_libraries(test-logx cmocka cx)
add_test(test-logx ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-logx)

add_executable(test-optx test_optx.c)
target_link_libraries(test-optx cmocka cx)
add_test(test-optx ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-optx)

add_executable(test-ringbufx test_ringbufx.c)
target_link_libraries(test-ringbufx cmocka cx)
add_test(test-ringbufx ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-ringbufx)

add_executable(test-autobufx test_autobufx.c)
target_link_libraries(test-autobufx cmocka cx)
add_test(test-autobufx ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-autobufx)

add_executable(test-apix test_apix.c)
target_link_libraries(test-apix cmocka cx pthread)
add_test(test-apix ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-apix)

if (BUILD_POSIX)
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
endif ()
