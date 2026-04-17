add_executable(test_proc_piped_cat
    proc/test_proc_piped_cat.cpp
)
target_link_libraries(test_proc_piped_cat PRIVATE pcr::proc)
target_include_directories(test_proc_piped_cat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME test_proc_piped_cat COMMAND test_proc_piped_cat)

# add_executable(test_proc_piped_env
#   test_proc_piped_env.cpp
# )
# target_link_libraries(test_proc_piped_env PRIVATE pcr::proc)
# target_include_directories(test_proc_piped_env PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# add_test(NAME test_proc_piped_env COMMAND test_proc_piped_env)
#
# add_executable(test_proc_generic_redirection
#   test_proc_generic_redirection.cpp
# )
# target_link_libraries(test_proc_generic_redirection PRIVATE pcr::proc)
# target_include_directories(test_proc_generic_redirection PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# add_test(NAME test_proc_generic_redirection COMMAND test_proc_generic_redirection)
#
# add_executable(test_proc_generic_cwd
#   test_proc_generic_cwd.cpp
# )
# target_link_libraries(test_proc_generic_cwd PRIVATE pcr::proc)
# target_include_directories(test_proc_generic_cwd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# add_test(NAME test_proc_generic_cwd COMMAND test_proc_generic_cwd)
#
add_executable(test_proc_terminate
    proc/test_proc_terminate.cpp
)
target_link_libraries(test_proc_terminate PRIVATE pcr::proc)
target_include_directories(test_proc_terminate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME test_proc_terminate COMMAND test_proc_terminate)

add_executable(test_proc_spawn_fail
    proc/test_proc_spawn_fail.cpp
)
target_link_libraries(test_proc_spawn_fail PRIVATE pcr::proc)
target_include_directories(test_proc_spawn_fail PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME test_proc_spawn_fail COMMAND test_proc_spawn_fail)

add_executable(test_stream_proc_pipes
    stream/test_stream_proc_pipes.cpp
)
target_link_libraries(test_stream_proc_pipes PRIVATE pcr::stream pcr::proc)
target_include_directories(test_stream_proc_pipes PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME test_stream_proc_pipes COMMAND test_stream_proc_pipes)

#
#
add_executable(test_stream_socketpair
  stream/test_stream_socketpair.cpp
)
target_link_libraries(test_stream_socketpair PRIVATE pcr::stream)
target_include_directories(test_stream_socketpair PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME test_stream_socketpair COMMAND test_stream_socketpair)

# add_executable(test_stream_read_exact
#   test_stream_read_exact.cpp
# )
# target_link_libraries(test_stream_read_exact PRIVATE pcr::stream)
# add_test(NAME test_stream_read_exact COMMAND test_stream_read_exact)
#
# add_executable(test_stream_invalid_fd
#   test_stream_invalid_fd.cpp
# )
# target_link_libraries(test_stream_invalid_fd PRIVATE pcr::stream)
# add_test(NAME test_stream_invalid_fd COMMAND test_stream_invalid_fd)


# framing tests


add_executable(test_framing_content_length_two_frames
    framing/test_framing_content_length_two_frames.cpp
)
target_link_libraries(test_framing_content_length_two_frames PRIVATE pcr::framing)
add_test(NAME test_framing_content_length_two_frames COMMAND test_framing_content_length_two_frames)


add_executable(test_framing_content_length_pipes_two_frames
    framing/test_framing_content_length_pipes_two_frames.cpp
)
target_link_libraries(test_framing_content_length_pipes_two_frames PRIVATE pcr::framing)
add_test(NAME test_framing_content_length_pipes_two_frames COMMAND test_framing_content_length_pipes_two_frames)





# jsonrpc tests

add_executable(test_jsonrpc_content_length_pipes_roundtrip    
    jsonrpc/test_jsonrpc_content_length_pipes_roundtrip.cpp
)
target_link_libraries(test_jsonrpc_content_length_pipes_roundtrip PRIVATE pcr::jsonrpc_nlohmann)
add_test(NAME test_jsonrpc_content_length_pipes_roundtrip COMMAND test_jsonrpc_content_length_pipes_roundtrip)

