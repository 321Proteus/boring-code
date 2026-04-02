cmake_minimum_required(VERSION 3.16)
project(BoringCode LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
add_subdirectory(src)
add_subdirectory(tools)