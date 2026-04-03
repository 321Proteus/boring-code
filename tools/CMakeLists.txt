cmake_minimum_required(VERSION 3.16)
project(BoringTool C CXX)
set(CMAKE_C_STANDARD 11)

message(STATUS "Pointer size: ${CMAKE_SIZEOF_VOID_P}")

find_package(DynamoRIO REQUIRED)
find_package(ZLIB REQUIRED)

add_library(boringtool SHARED main.c)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_target_properties(boringtool PROPERTIES OUTPUT_NAME "boringtool64")
else()
    set_target_properties(boringtool PROPERTIES OUTPUT_NAME "boringtool")
endif()

set(DR_EXT "/opt/dynamorio/ext")
target_include_directories(boringtool PRIVATE ${DR_EXT}/include)
# target_link_directories(boringtool PRIVATE ${DR_EXT}/lib64)
target_link_libraries(boringtool drmgr z)

configure_DynamoRIO_client(boringtool)
