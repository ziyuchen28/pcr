
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



# jsonrpc
add_library(pcr_jsonrpc
  codec.cpp
  codec_nlohmann.cpp
  peer.cpp
  dispatcher.cpp
)
add_library(pcr::jsonrpc ALIAS pcr_jsonrpc)

target_include_directories(pcr_jsonrpc PUBLIC ${PCR_ROOT_DIR})
target_compile_features(pcr_jsonrpc PUBLIC cxx_std_20)
if(MSVC)
  target_compile_options(pcr_jsonrpc PRIVATE /W4 /permissive-)
else()
  target_compile_options(pcr_jsonrpc PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_link_libraries(pcr_jsonrpc 
  PUBLIC pcr::framing
  PRIVATE nlohmann_json::nlohmann_json
)


