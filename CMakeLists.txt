cmake_minimum_required(VERSION 3.22)

# # # # # # # # # # # # # # # # # # # # # # #
#     Project name, version and settings    #
# # # # # # # # # # # # # # # # # # # # # # #
project(pigaco LANGUAGES C CXX)

set(PIGACO_MAJOR_VERSION 1)
set(PIGACO_MINOR_VERSION 1)
set(PIGACO_PATCH_VERSION 5)

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
#      Targets                              #
# # # # # # # # # # # # # # # # # # # # # # #
add_executable(${PROJECT_NAME}c main.c)
add_executable(${PROJECT_NAME}cxx main.cc)

target_include_directories(${PROJECT_NAME}c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}c PUBLIC m pthread)

target_include_directories(${PROJECT_NAME}cxx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}cxx PUBLIC m pthread)
