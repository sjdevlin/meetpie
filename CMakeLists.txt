project(meetpie)
cmake_minimum_required(VERSION 2.4.6)

  if(COMMAND cmake_policy)
      cmake_policy(SET CMP0003 NEW)
    endif(COMMAND cmake_policy)

find_package(PkgConfig REQUIRED)
pkg_search_module (PC_JSON-C REQUIRED json-c json)
pkg_check_modules(PC_GLIB REQUIRED glib-2.0)
pkg_check_modules(PC_GIO REQUIRED gio-2.0)
pkg_check_modules(PC_GOB REQUIRED gobject-2.0)

#find_library(MATRIX_CREATOR_HAL matrix_creator_hal)

include_directories("${PROJECT_SOURCE_DIR}/include")

set (SOURCES src/meetpie.cpp src/json_parsing.c)
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/bin")

#Add base directory for includes (global)

find_path (JSON_C_INCLUDE_DIR json.h PATH_SUFFIXES json-c json)
find_library (JSON_C_LIBRARIES NAMES json-c libjson-c)
include_directories (${JSON_C_INCLUDE_DIR} ${PROJECT_SOURCE_DIR/include})


add_executable(meetpie
    ${SOURCES}
)

target_link_libraries(meetpie
    libc.so.6
    glib-2.0
    gio-2.0
    gobject-2.0
    ${JSON_C_LIBRARIES}
#    ${MATRIX_CREATOR_HAL}
    libm.so.6
    ${PROJECT_SOURCE_DIR}/lib/libggk.a
)

install(TARGETS meetpie DESTINATION bin)
