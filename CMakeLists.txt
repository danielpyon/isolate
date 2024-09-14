cmake_minimum_required(VERSION 3.27)
project(isolate)
set(TARGET isolate)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Curses REQUIRED)

# Add the executable
add_executable(${TARGET} src/isolate.cpp)

target_link_libraries(${TARGET} PRIVATE ${CURSES_LIBRARIES})
target_include_directories(${TARGET} PRIVATE ${CURSES_INCLUDE_DIR})

if(APPLE)
    target_link_options(${TARGET} PRIVATE LINKER:-sectcreate,__TEXT,__info_plist,${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
    add_custom_command(TARGET ${TARGET}
        POST_BUILD
        COMMAND /bin/sh -c "codesign -s isolate ./isolate"
    )
endif()
