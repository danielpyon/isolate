cmake_minimum_required(VERSION 3.27)
project(isolate)
set(TARGET isolate)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(XNU_PATH "${CMAKE_SOURCE_DIR}/external/xnu")

find_package(Curses REQUIRED)

# Add the executable
add_executable(${TARGET} src/isolate.cpp ${XNU_PATH}/osfmk/mach/mach_exc.defs)

target_link_libraries(${TARGET} PRIVATE ${CURSES_LIBRARIES})
target_include_directories(${TARGET} PRIVATE ${CURSES_INCLUDE_DIR})

if(APPLE)
    target_link_options(${TARGET} PRIVATE LINKER:-sectcreate,__TEXT,__info_plist,${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)

    set(CERT CACHE STRING "the name of the certificate for code-signing")

    if(NOT DEFINED CERT OR CERT STREQUAL "")
        message(FATAL_ERROR "CERT must be set to the name of the certificate for code-signing. Please set it using -DCERT=<value>.")
    endif()

    set(CODE_SIGN "codesign -s ${CERT} ./isolate")
    message(STATUS "Signing with: ${CODE_SIGN}")

    add_custom_command(TARGET ${TARGET}
        POST_BUILD
        COMMAND /bin/sh -c ${CODE_SIGN} 
    )
endif()
