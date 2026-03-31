cmake_minimum_required(VERSION 4.2)
project(gpu_display)

set(CMAKE_CXX_STANDARD 20)

add_library(gpu_display SHARED
        src/gpu_monitor.cpp
        src/amd/amd_monitor.cpp
        src/nvidia/nvidia_monitor.cpp)

target_include_directories(gpu_display PRIVATE
        src
        lib/adl/include)