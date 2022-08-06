cmake_minimum_required(VERSION 3.12)

file(GLOB SRC *.c)
file(GLOB INC *.h)
if (BUILD_POSIX)
    file(GLOB SRC_POSIX posix/*.c)
    file(GLOB INC_POSIX posix/*.h)
endif ()

if (BUILD_STATIC)
    add_library(cx-static STATIC ${SRC} ${SRC_POSIX})
    set_target_properties(cx-static PROPERTIES OUTPUT_NAME cx)
    set_target_properties(cx-static PROPERTIES PUBLIC_HEADER "${INC}")
    set(TARGET_STATIC cx-static)
endif ()

if (BUILD_SHARED)
    add_library(cx SHARED ${SRC} ${SRC_POSIX})
    set_target_properties(cx PROPERTIES PUBLIC_HEADER "${INC}")
    set_target_properties(cx PROPERTIES VERSION 0.0.1 SOVERSION 0)
    set(TARGET_SHARED cx)
endif ()

install(TARGETS ${TARGET_STATIC} ${TARGET_SHARED}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/ext" # source directory
        DESTINATION "include" # target directory
        FILES_MATCHING # install only matched files
        PATTERN "*.h" # select header files
)
