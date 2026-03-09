add_library(pcr_rpc_core
  src/encode.cpp
  src/peer.cpp
  src/dispatcher.cpp
)

add_library(pcr::rpc_core ALIAS pcr_rpc_core)

target_include_directories(pcr_rpc_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(pcr_rpc_core PUBLIC cxx_std_20)

target_compile_options(pcr_rpc_core PRIVATE
  -Wall -Wextra -Wpedantic
)

target_link_libraries(pcr_rpc_core PUBLIC pcr::framing)
