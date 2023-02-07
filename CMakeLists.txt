cmake_minimum_required(VERSION 3.12)
project(EternalPatcher)

set(CMAKE_C_STANDARD 99)

file(GLOB SOURCES
        ./cvector/cvector.h
        ./cvector/cvector_utils.h
        ./eternalpatcher.h
        ./main.c
        ./patcher.c
        ./utils.c
        ./apply.c
        ./curl.c
        )

find_package(PkgConfig REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL "3.0" REQUIRED)

set (CMAKE_C_FLAGS "-O2 -DNDEBUG")
add_executable(EternalPatcher ${SOURCES})

target_link_libraries(EternalPatcher CURL::libcurl OpenSSL::Crypto ${GLIB_LDFLAGS})
