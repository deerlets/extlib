cmake_minimum_required(VERSION 3.12)

add_executable(test-err test_err.c)
target_link_libraries(test-err cmocka cx)
add_test(test-err ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-err)

add_executable(test-log test_log.c)
target_link_libraries(test-log cmocka cx)
add_test(test-log ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-log)

add_executable(test-opt test_opt.c)
target_link_libraries(test-opt cmocka cx)
add_test(test-opt ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-opt)

add_executable(test-ringbuf test_ringbuf.c)
target_link_libraries(test-ringbuf cmocka cx)
add_test(test-ringbuf ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-ringbuf)

add_executable(test-atbuf test_atbuf.c)
target_link_libraries(test-atbuf cmocka cx)
add_test(test-atbuf ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-atbuf)

add_executable(test-srrp test_srrp.c)
target_link_libraries(test-srrp cmocka cx pthread)
add_test(test-srrp ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-srrp)

add_executable(test-json test_json.c)
target_link_libraries(test-json cmocka cx pthread)
add_test(test-json ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-json)

add_executable(test-apix test_apix.c)
target_link_libraries(test-apix cmocka cx pthread)
add_test(test-apix ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-apix)

add_executable(test-svcx test_svcx.c)
target_link_libraries(test-svcx cmocka cx pthread)
add_test(test-svcx ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-svcx)

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
