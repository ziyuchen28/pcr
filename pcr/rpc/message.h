#pragma once

#include "error.h"
#include "id.h"

#include <optional>
#include <string>
#include <variant>

namespace pcr::rpc {

// params/result are stored as raw JSON text (value), so rpc_core stays JSON-library-free
struct Request 
{
    Id id;
    std::string method;
    std::optional<std::string> params_json;
};

struct Notification 
{
    std::string method;
    std::optional<std::string> params_json;
};

struct Response 
{
    Id id;
    // JSON-RPC 2.0 spec - 
    // A response can be a Success, or it can be a Failure. 
    // It can never be both, and it can never be neither
    std::optional<std::string> result_json; 
    std::optional<Error> error;
};

using Message = std::variant<Request, Notification, Response>;

} // namespace pcr::rpc
