

add_library(pcr_ipc_stdio_jsonrpc stdio_jsonrpc_session.cpp)
add_library(pcr::ipc_stdio_jsonrpc ALIAS pcr_ipc_stdio_jsonrpc)

target_include_directories(pcr_ipc_stdio_jsonrpc PUBLIC ${PCR_ROOT_DIR})

target_compile_features(pcr_ipc_stdio_jsonrpc PUBLIC cxx_std_20)
if(MSVC)
  target_compile_options(pcr_ipc_stdio_jsonrpc PRIVATE /W4 /permissive-)
else()
  target_compile_options(pcr_ipc_stdio_jsonrpc PRIVATE -Wall -Wextra -Wpedantic)
endif()


target_link_libraries(pcr_ipc_stdio_jsonrpc
    PRIVATE
        pcr::proc
    PUBLIC
        pcr::jsonrpc
)
