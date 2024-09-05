cmake_minimum_required(VERSION 3.27)
project(isolate)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Curses REQUIRED)

# Add the executable
add_executable(isolate isolate.cpp)

target_link_libraries(isolate PRIVATE ${CURSES_LIBRARIES})
target_include_directories(isolate PRIVATE ${CURSES_INCLUDE_DIR})
