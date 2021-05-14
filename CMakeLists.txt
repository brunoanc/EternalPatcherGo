cmake_minimum_required(VERSION 3.12)
project(EternalPatcher)

set(CMAKE_C_STANDARD 99)

file(GLOB SOURCES
        ./main.c
        ./patcher.c
        ./utils.c
        ./apply.c
        ./curl.c
        )

find_package(PkgConfig REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

pkg_check_modules(GLIB REQUIRED glib-2.0)
include_directories(${GLIB_INCLUDE_DIRS})

set (CMAKE_C_FLAGS "-O2 -DNDEBUG")
add_executable(EternalPatcher ${SOURCES})

target_link_libraries(EternalPatcher CURL::libcurl OpenSSL::Crypto ${GLIB_LDFLAGS})