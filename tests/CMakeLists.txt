add_executable(test_proc_piped_cat
    test_proc_piped_cat.cpp
)
target_link_libraries(test_proc_piped_cat PRIVATE prism::proc)
target_include_directories(test_proc_piped_cat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME test_proc_piped_cat COMMAND test_proc_piped_cat)

# add_executable(test_proc_piped_env
#   test_proc_piped_env.cpp
# )
# target_link_libraries(test_proc_piped_env PRIVATE prism::proc)
# target_include_directories(test_proc_piped_env PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# add_test(NAME test_proc_piped_env COMMAND test_proc_piped_env)
#
# add_executable(test_proc_generic_redirection
#   test_proc_generic_redirection.cpp
# )
# target_link_libraries(test_proc_generic_redirection PRIVATE prism::proc)
# target_include_directories(test_proc_generic_redirection PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# add_test(NAME test_proc_generic_redirection COMMAND test_proc_generic_redirection)
#
# add_executable(test_proc_generic_cwd
#   test_proc_generic_cwd.cpp
# )
# target_link_libraries(test_proc_generic_cwd PRIVATE prism::proc)
# target_include_directories(test_proc_generic_cwd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# add_test(NAME test_proc_generic_cwd COMMAND test_proc_generic_cwd)
#
add_executable(test_proc_terminate
  test_proc_terminate.cpp
)
target_link_libraries(test_proc_terminate PRIVATE prism::proc)
target_include_directories(test_proc_terminate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME test_proc_terminate COMMAND test_proc_terminate)

add_executable(test_proc_spawn_fail
  test_proc_spawn_fail.cpp
)
target_link_libraries(test_proc_spawn_fail PRIVATE prism::proc)
target_include_directories(test_proc_spawn_fail PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME test_proc_spawn_fail COMMAND test_proc_spawn_fail)
