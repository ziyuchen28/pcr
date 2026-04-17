

add_library(pcr_ipc child_ipc_posix.cpp)
add_library(pcr::ipc ALIAS pcr_ipc)

target_include_directories(pcr_ipc PUBLIC ${PCR_ROOT_DIR})

target_compile_features(pcr_ipc PUBLIC cxx_std_20)
target_compile_options(pcr_ipc PRIVATE -Wall -Wextra -Wpedantic)


