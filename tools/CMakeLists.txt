project(BoringTool)

find_package(DynamoRIO REQUIRED)
find_package(ZLIB REQUIRED)

add_library(boringtool SHARED main.c)

set(DR_EXT "/opt/dynamorio/ext")
target_include_directories(boringtool PRIVATE ${DR_EXT}/include)
target_link_directories(boringtool PRIVATE ${DR_EXT}/lib64)
target_link_libraries(boringtool drmgr)
target_link_libraries(boringtool z)

configure_DynamoRIO_client(boringtool)
