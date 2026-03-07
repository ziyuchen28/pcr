
add_library(pcr_channel
    src/pipe_stream.cpp
    src/socket_stream.cpp
)
add_library(pcr::channel ALIAS pcr_channel)

target_include_directories(pcr_channel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(pcr_channel PUBLIC cxx_std_20)
target_compile_options(pcr_channel PRIVATE -Wall -Wextra -Wpedantic)
