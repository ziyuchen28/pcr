
if(WIN32)
  set(PCR_STREAM_SOURCES
    pipe_stream_win.cpp
    # socket_stream_win.cpp 
  )
else()
  set(PCR_STREAM_SOURCES
    pipe_stream_posix.cpp
    socket_stream_posix.cpp
  )
endif()
add_library(pcr::stream ALIAS pcr_stream)

target_include_directories(pcr_stream PUBLIC ${PCR_ROOT_DIR})

target_compile_features(pcr_stream PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(pcr_stream PRIVATE /W4 /permissive-)
else()
  target_compile_options(pcr_stream PRIVATE -Wall -Wextra -Wpedantic)
endif()
