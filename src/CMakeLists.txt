cmake_minimum_required(VERSION 2.8)

file(GLOB SRC *.c)
file(GLOB INC *.h)

if (BUILD_STATIC)
    add_library(cx-static STATIC ${SRC})
    set_target_properties(cx-static PROPERTIES OUTPUT_NAME cx)
    set_target_properties(cx-static PROPERTIES PUBLIC_HEADER "${INC}")
    set(TARGET_STATIC cx-static)
endif ()

if (BUILD_SHARED)
    add_library(cx SHARED ${SRC})
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
