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
    ui/*.cpp
    ui/*.hpp
    ui/*.h
    data/*.hpp
)

file(GLOB_RECURSE APP_QT_SOURCES
    app/*.cpp
    app/*.hpp
    app/*.h
    app/*.ui
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

add_executable(BoringCodeCLI
    ${APP_CLI_SOURCES}
    ${APP_COMMON_SOURCES}
    ${APP_PROVIDER_DIRECT}
    ${CORE_SOURCES}
)

add_executable(BoringCode
    ${APP_QT_SOURCES}
    ${APP_COMMON_SOURCES}
    ${APP_PROVIDER_DIRECT}
    ${CORE_SOURCES}
)

target_link_libraries(BoringCode Qt6::Widgets ZLIB::ZLIB ${CAPSTONE_LIBRARIES})

target_include_directories(BoringCode PRIVATE ${CAPSTONE_INCLUDE_DIRS})

target_include_directories(BoringCode PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(BoringCodeCLI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})