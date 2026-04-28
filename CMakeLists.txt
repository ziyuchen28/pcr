cmake_minimum_required(VERSION 3.24)
project(pcr VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PCR_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "pcr root")

if(PROJECT_IS_TOP_LEVEL)
    option(PCR_BUILD_TESTS "Build pcr tests" ON)
else()
    option(PCR_BUILD_TESTS "Build pcr tests" OFF)
endif()

# add_subdirectory(libs/proc)
# add_subdirectory(libs/channel)
# add_subdirectory(libs/framing)
# add_subdirectory(libs/rpc)


add_subdirectory(pcr/proc)
add_subdirectory(pcr/stream)
add_subdirectory(pcr/framing)
add_subdirectory(pcr/jsonrpc)

if(NOT WIN32)
add_subdirectory(pcr/ipc)
endif()

if(PCR_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
