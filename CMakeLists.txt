cmake_minimum_required(VERSION 3.22)

# # # # # # # # # # # # # # # # # # # # # # #
#     Project name and version              #
# # # # # # # # # # # # # # # # # # # # # # #
project(pigaco LANGUAGES C)

set(PIGACO_MAJOR_VERSION 1)
set(PIGACO_MINOR_VERSION 0)
set(PIGACO_PATCH_VERSION 0)

set(PIGACO_VERSION
    ${PIGACO_MAJOR_VERSION}.${PIGACO_MINOR_VERSION}.${PIGACO_PATCH_VERSION})

set(PIGACO_VERSION_STRING "${PIGACO_VERSION}")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# # # # # # # # # # # # # # # # # # # # # # #
#     Clangd setups                         #
# # # # # # # # # # # # # # # # # # # # # # #
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(CREATE_LINK
  "${CMAKE_BINARY_DIR}/compile_commands.json"
  "${CMAKE_SOURCE_DIR}/compile_commands.json"
  SYMBOLIC
)

# # # # # # # # # # # # # # # # # # # # # # #
#     ...                                   #
# # # # # # # # # # # # # # # # # # # # # # #
# configure_file(
#   ${PROJECT_SOURCE_DIR}/version.h.in
#   ${PROJECT_SOURCE_DIR}/version.h
#   @ONLY
# )

# list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

# include(FindPackages)

# # # # # # # # # # # # # # # # # # # # # # #
#     Subdirectories                        #
# # # # # # # # # # # # # # # # # # # # # # #
# add_subdirectory(src)

add_executable(${PROJECT_NAME} main.c)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME} PUBLIC m pthread)
