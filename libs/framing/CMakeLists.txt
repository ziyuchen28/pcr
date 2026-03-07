add_library(pcr_framing
    src/content_length_framer.cpp
    src/ndjson_framer.cpp
    src/length_prefix_framer.cpp
)

add_library(pcr::framing ALIAS pcr_framing)

target_include_directories(pcr_framing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(pcr_framing PUBLIC cxx_std_20)

target_compile_options(pcr_framing PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(pcr_framing PUBLIC pcr::channel)
