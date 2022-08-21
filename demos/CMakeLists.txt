cmake_minimum_required(VERSION 3.12)

add_executable(apix_serial apix_serial.c)
target_link_libraries(apix_serial cx)
add_test(apix_serial ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/apix_serial)

add_executable(apix_unix_tcp_serial apix_unix_tcp_serial.c)
target_link_libraries(apix_unix_tcp_serial cx)
add_test(apix_unix_tcp_serial ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/apix_unix_tcp_serial)
