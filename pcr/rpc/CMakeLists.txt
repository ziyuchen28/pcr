
# core
add_library(pcr_rpc_core
  encode.cpp
  peer.cpp
  dispatcher.cpp
)
add_library(pcr::rpc_core ALIAS pcr_rpc_core)

target_include_directories(pcr_rpc_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(pcr_rpc_core PUBLIC cxx_std_20)
target_compile_options(pcr_rpc_core PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(pcr_rpc_core PUBLIC pcr::framing)


# nlohmann 
# Try to locate nlohmann target in three ways:
# 1) parent/toolchain already provided nlohmann_json::nlohmann_json
# 2) vendored source at external/nlohmann_json (git submodule or copied)
# 3) FetchContent (pinned tarball) if PCR_FETCH_DEPS=ON
if (TARGET nlohmann_json::nlohmann_json)
  # ok
elseif (EXISTS "${PCR_ROOT_DIR}/external/nlohmann_json/CMakeLists.txt")
  set(JSON_Install OFF CACHE INTERNAL "")
  set(JSON_BuildTests OFF CACHE INTERNAL "")
  add_subdirectory(
    "${PCR_ROOT_DIR}/external/nlohmann_json"
    "${CMAKE_BINARY_DIR}/_deps/nlohmann_json-build"
    EXCLUDE_FROM_ALL
  )
elseif (PCR_FETCH_DEPS)
  include(FetchContent)
  FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
  )
  FetchContent_MakeAvailable(nlohmann_json)
else()
  message(FATAL_ERROR
    "nlohmann_json::nlohmann_json not found.\n"
    "Provide it via toolchain/package manager, vendor under external/nlohmann_json,\n"
    "or enable PCR_FETCH_DEPS=ON.")
endif()


# nlohmann backed codec 

add_library(pcr_rpc_codec_nlohmann
    codec/nlohmann.cpp
)
add_library(pcr::rpc_codec_nlohmann ALIAS pcr_rpc_codec_nlohmann)
target_include_directories(pcr_rpc_codec_nlohmann PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/codec
)
target_compile_features(pcr_rpc_codec_nlohmann PUBLIC cxx_std_20)
target_compile_options(pcr_rpc_codec_nlohmann PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(pcr_rpc_codec_nlohmann
    PUBLIC
        pcr::rpc_core
        nlohmann_json::nlohmann_json
)

# wrapper interface
add_library(pcr_rpc_nlohmann INTERFACE)
add_library(pcr::rpc_nlohmann ALIAS pcr_rpc_nlohmann)
target_link_libraries(pcr_rpc_nlohmann INTERFACE
    pcr::rpc_core
    pcr::rpc_codec_nlohmann
)
