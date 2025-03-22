cmake_minimum_required(VERSION 3.22)

project(pigaco LANGUAGES C CXX)

set(PIGACO_MAJOR_VERSION 1)
set(PIGACO_MINOR_VERSION 1)
set(PIGACO_PATCH_VERSION 5)

set(PIGACO_VERSION
    ${PIGACO_MAJOR_VERSION}.${PIGACO_MINOR_VERSION}.${PIGACO_PATCH_VERSION})

set(PIGACO_VERSION_STRING "${PIGACO_VERSION}")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(CREATE_LINK "${CMAKE_BINARY_DIR}/compile_commands.json"
     "${CMAKE_SOURCE_DIR}/compile_commands.json" SYMBOLIC)

# find_package(PkgConfig REQUIRED) pkg_check_modules( FFMPEG REQUIRED
# IMPORTED_TARGET libavcodec libavformat libavutil libswscale)

add_executable(${PROJECT_NAME}c main.c)
add_executable(${PROJECT_NAME}cxx main.cc)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}c PRIVATE m pthread)

target_link_libraries(${PROJECT_NAME}cxx PRIVATE m pthread) # PkgConfig::FFMPEG
                                                            # atomic

# target_compile_options(video PRIVATE -mavx2)
