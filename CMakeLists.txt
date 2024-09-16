cmake_minimum_required(VERSION 3.27)
project(isolate)
set(TARGET isolate)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Curses REQUIRED)

set(DEF_FILE "${CMAKE_SOURCE_DIR}/external/xnu/osfmk/mach/mach_exc.defs")
set(GENERATED_HEADERS ${CMAKE_SOURCE_DIR}/include/mach_exc.h)
set(GENERATED_SOURCES
    ${CMAKE_SOURCE_DIR}/include/mach_excServer.c
    ${CMAKE_SOURCE_DIR}/include/mach_excUser.c
)

# Add a custom command to run `mig` and generate the `.c` source files
add_custom_command(
    OUTPUT ${GENERATED_HEADERS} ${GENERATED_SOURCES}
    COMMAND mig ${DEF_FILE}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/include
    COMMENT "Running mig to generate mach_exc source files"
    DEPENDS ${DEF_FILE}
)

add_executable(${TARGET}
    ${CMAKE_SOURCE_DIR}/src/isolate.cpp
    ${CMAKE_SOURCE_DIR}/src/mach_exc_handlers.cpp

    ${CMAKE_SOURCE_DIR}/include/mach_exc_handlers.h
    ${CMAKE_SOURCE_DIR}/include/mach_exc.h
    ${CMAKE_SOURCE_DIR}/include/mach_excServer.c
    ${CMAKE_SOURCE_DIR}/include/mach_excUser.c
)
target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(${TARGET} PRIVATE ${CURSES_LIBRARIES})
target_include_directories(${TARGET} PRIVATE ${CURSES_INCLUDE_DIR})

if(APPLE)
    target_link_options(${TARGET} PRIVATE LINKER:-sectcreate,__TEXT,__info_plist,${CMAKE_SOURCE_DIR}/Info.plist)

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
