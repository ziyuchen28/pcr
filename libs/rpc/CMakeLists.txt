add_subdirectory(core)
add_subdirectory(codec/nlohmann)

# Convenience bundle: "give me rpc core + nlohmann codec"
add_library(pcr_rpc_nlohmann INTERFACE)
add_library(pcr::rpc_nlohmann ALIAS pcr_rpc_nlohmann)
target_link_libraries(pcr_rpc_nlohmann INTERFACE
  pcr::rpc_core
  pcr::rpc_codec_nlohmann
)
