#pragma once

#include "pcr/rpc/error.h"
#include "pcr/rpc/id.h"

#include <optional>
#include <string>
#include <variant>

namespace pcr::rpc {

// params/result are stored as raw JSON text (value), so rpc_core stays JSON-library-free.
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
    std::optional<std::string> result_json; // raw JSON value text
    std::optional<Error> error;             // exactly one of result_json or error should be set
};

using Message = std::variant<Request, Notification, Response>;

} // namespace pcr::rpc
