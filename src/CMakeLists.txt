find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(ZLIB REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CAPSTONE REQUIRED capstone)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

file(GLOB_RECURSE CORE_SOURCES
    core/*.cpp
    core/*.hpp
    core/*.h
)

file(GLOB_RECURSE APP_COMMON_SOURCES
    data/*.hpp
)

file(GLOB_RECURSE APP_QT_SOURCES
    gui/*.cpp
    gui/*.hpp
    gui/*.h
    gui/*.ui
)

file(GLOB_RECURSE APP_CLI_SOURCES
    cli/*.cpp
    cli/*.hpp
    cli/*.h
)

if (MSVC)
    set(APP_PROVIDER_DIRECT providers/direct.hpp providers/direct_win.cpp)
else()
    set(APP_PROVIDER_DIRECT providers/direct.hpp providers/direct_unix.cpp)
endif()

add_library(BoringCodeCore STATIC ${CORE_SOURCES})
add_library(BoringCodeAppCommon STATIC ${APP_COMMON_SOURCES})
add_library(BoringCodeDirect STATIC ${APP_PROVIDER_DIRECT})

target_include_directories(BoringCodeAppCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(BoringCodeDirect PRIVATE BoringCodeCore)
target_link_libraries(BoringCodeDirect PRIVATE BoringCodeAppCommon)

target_link_libraries(BoringCodeAppCommon PRIVATE BoringCodeCore)
target_link_libraries(BoringCodeAppCommon PRIVATE BoringCodeDirect)

add_executable(BoringCodeCLI ${APP_CLI_SOURCES})
target_link_libraries(BoringCodeCLI PRIVATE BoringCodeAppCommon)

add_subdirectory(gui)