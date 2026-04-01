cmake_minimum_required(VERSION 3.16)

project(BoringCode LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

file(GLOB_RECURSE CORE_SOURCES
    src/core/*.cpp
    src/core/*.hpp
    src/core/*.h
)

file(GLOB_RECURSE APP_COMMON_SOURCES
    src/ui/*.cpp
    src/ui/*.hpp
    src/ui/*.h
)

file(GLOB_RECURSE APP_QT_SOURCES
    src/app/*.cpp
    src/app/*.hpp
    src/app/*.h
    src/app/*.ui
)

file(GLOB_RECURSE APP_CLI_SOURCES
    src/cli/*.cpp
    src/cli/*.hpp
    src/cli/*.h
)

add_executable(BoringCodeCLI
    ${APP_CLI_SOURCES}
    ${APP_COMMON_SOURCES}
    ${CORE_SOURCES}
)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(BoringCode
    ${APP_QT_SOURCES}
    ${APP_COMMON_SOURCES}
    ${CORE_SOURCES}
)

target_link_libraries(BoringCode Qt6::Widgets)

target_include_directories(BoringCode PRIVATE src)
target_include_directories(BoringCodeCLI PRIVATE src)