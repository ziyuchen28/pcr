#pragma once

#include <optional>
#include <string>

namespace pcr::jsonrpc {

struct Error 
{
    int code = 0;
    std::string message;
    std::optional<std::string> data_json; // raw JSON value text
};


// standard JSON-RPC codes
inline constexpr int kParseError     = -32700;
inline constexpr int kInvalidRequest = -32600;
inline constexpr int kMethodNotFound = -32601;
inline constexpr int kInvalidParams  = -32602;
inline constexpr int kInternalError  = -32603;

} // namespace pcr::jsonrpc
