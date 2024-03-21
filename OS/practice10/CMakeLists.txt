cmake_minimum_required(VERSION 3.26)
project(OS-P10)

set(CMAKE_CXX_STANDARD 20)

add_executable(OS-P10 main.cpp)

find_path(LIBEV_INCLUDE_DIR NAMES ev.h)
find_library(LIBEV_LIBRARY NAMES ev)
if (LIBEV_INCLUDE_DIR AND LIBEV_LIBRARY)
    include_directories(${LIBEV_INCLUDE_DIR})
    target_link_libraries(OS-P10 ${LIBEV_LIBRARY})
else()
    message(FATAL_ERROR "libev not found")
endif()