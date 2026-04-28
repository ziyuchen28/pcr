add_library(pcr_framing
    content_length_framer.cpp
    ndjson_framer.cpp
    length_prefix_framer.cpp
)

add_library(pcr::framing ALIAS pcr_framing)

target_include_directories(pcr_framing PUBLIC ${PCR_ROOT_DIR})

target_compile_features(pcr_framing PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(pcr_framing PRIVATE /W4 /permissive-)
else()
  target_compile_options(pcr_framing PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(pcr_framing PUBLIC pcr::stream)
