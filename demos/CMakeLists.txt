cmake_minimum_required(VERSION 3.12)

add_executable(apix_serial apix_serial.c)
target_link_libraries(apix_serial cx)

add_executable(apix_tcp apix_tcp.c)
target_link_libraries(apix_tcp cx)

add_executable(apix_unix_tcp_serial apix_unix_tcp_serial.c)
target_link_libraries(apix_unix_tcp_serial cx)
