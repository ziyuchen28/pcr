#pragma once

#include "message.h"

#include <nlohmann/json.hpp>

#include <string>

namespace pcr::rpc {

// helper for handlers/tests: build raw JSON value text easily
inline std::string json_dump(const nlohmann::json &j) 
{
    return j.dump(); // compact
}

// Nlohmann-based JSON-RPC codec:
// - decode: uses nlohmann::json parse
// - encode: uses rpc_core manual encoder (encode_message_json)
class NlohmannCodec 
{
public:
    Message decode(std::string &&payload);
    std::string encode(const Message &msg);
};

} // namespace pcr::rpc
