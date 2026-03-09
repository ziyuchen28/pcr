option(PCR_FETCH_DEPS "Allow CMake to download third-party deps with FetchContent" OFF)

if (TARGET nlohmann_json::nlohmann_json)
    # Already provided by parent project/toolchain.
elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../external/nlohmann_json/CMakeLists.txt")
    # Vendored copy (maybe a git submodule, maybe just copied source)
    set(JSON_Install OFF CACHE INTERNAL "")
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../external/nlohmann_json"
        "${CMAKE_BINARY_DIR}/_deps/nlohmann_json-build"
        EXCLUDE_FROM_ALL)
elseif (PCR_FETCH_DEPS)
    include(FetchContent)
    FetchContent_Declare(json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    )
    FetchContent_MakeAvailable(json)
else()
    message(FATAL_ERROR
        "nlohmann_json::nlohmann_json not found.\n"
        "Provide it via your toolchain/package manager, vendor under external/nlohmann_json,\n"
        "or enable PCR_FETCH_DEPS=ON.")
endif()
