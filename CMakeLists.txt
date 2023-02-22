cmake_minimum_required(VERSION 3.24)
project(hw5 C)
set(CMAKE_C_STANDARD 11)
#set(CMAKE_CXX_FLAGS "-lpthread")
add_executable(hw5
        scan.c)
target_link_libraries(hw5 m)
