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

if(DEFINED ENV{ETERNALPATCHERLINUX_STATIC})
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(CURL REQUIRED libcurl)
        pkg_check_modules(OPENSSL REQUIRED openssl)
        pkg_check_modules(BROTLICOMMON REQUIRED libbrotlicommon)
        set(CMAKE_C_FLAGS "${CURL_STATIC_CFLAGS} ${OPENSSL_STATIC_CFLAGS} ${BROTLICOMMON_STATIC_CFLAGS} -static -O2 -Wall -Werror -s")
else()
        find_package(CURL REQUIRED)
        find_package(OpenSSL "3.0" REQUIRED)
        set(CMAKE_C_FLAGS "-O2 -Wall -Werror -s")
endif()

add_executable(EternalPatcher ${SOURCES})

if(DEFINED ENV{ETERNALPATCHERLINUX_STATIC})
  target_link_libraries(EternalPatcher ${CURL_STATIC_LIBRARIES} ${OPENSSL_STATIC_LIBRARIES} ${BROTLICOMMON_STATIC_LIBRARIES})
else()
        target_link_libraries(EternalPatcher CURL::libcurl OpenSSL::Crypto)
endif()
