find_package(ZLIB REQUIRED)

if(WIN32)
    find_package(capstone CONFIG REQUIRED)
    set(CAPSTONE_TARGET capstone::capstone)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CAPSTONE REQUIRED IMPORTED_TARGET capstone)
    set(CAPSTONE_TARGET PkgConfig::CAPSTONE)
endif()

set(CMAKE_CXX_STANDARD 17)

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

if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
    set(APP_PROVIDER_DIRECT providers/direct.hpp providers/direct_win.cpp)
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    set(APP_PROVIDER_DIRECT providers/direct.hpp providers/direct_unix.cpp)
endif()

add_library(BoringCodeCore STATIC ${CORE_SOURCES})
add_library(BoringCodeAppCommon INTERFACE)
add_library(BoringCodeDirect STATIC ${APP_PROVIDER_DIRECT})

target_link_libraries(BoringCodeCore PUBLIC ${CAPSTONE_TARGET})
target_include_directories(BoringCodeCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(BoringCodeDirect PUBLIC BoringCodeCore)

target_link_libraries(BoringCodeAppCommon INTERFACE BoringCodeDirect)

add_executable(BoringCodeCLI ${APP_CLI_SOURCES})
target_link_libraries(BoringCodeCLI PRIVATE BoringCodeAppCommon)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_subdirectory(gui)
endif()

add_subdirectory(tools)