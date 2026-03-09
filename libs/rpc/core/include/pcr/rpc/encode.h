#pragma once

#include "pcr/rpc/message.h"

#include <string>

namespace pcr::rpc {

// Encodes Message -> JSON-RPC 2.0 UTF-8 JSON text.
// Inserts params/result/data as raw JSON value text (assumed valid JSON).
std::string encode_message_json(const Message& msg);

} // namespace pcr::rpc
