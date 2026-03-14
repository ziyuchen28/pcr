#include "pcr/rpc/codec/nlohmann.h"

#include "pcr/rpc/encode.h"
#include "pcr/rpc/error.h"
#include "pcr/rpc/id.h"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace pcr::rpc {

constexpr const char *k_json_rpc = "jsonrpc";
constexpr const char *k_json_rpc_ver = "2.0";


static void require_jsonrpc_2(const nlohmann::json &j) 
{
    if (!j.contains(k_json_rpc) || !j[k_json_rpc].is_string()) {
        throw std::runtime_error("rpc: missing/invalid jsonrpc field");
    }
    if (j[k_json_rpc].get<std::string>() != k_json_rpc_ver) {
        throw std::runtime_error("rpc: jsonrpc must be \"2.0\"");
    }
}


static Id parse_id(const nlohmann::json &v) 
{
    if (v.is_null()) return Id::null();

    if (v.is_number_integer()) {
        return Id::from_int(v.get<std::int64_t>());
    }

    if (v.is_number_unsigned()) {
        const auto u = v.get<std::uint64_t>();
        if (u > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
            throw std::runtime_error("rpc: id too large");
        }
        return Id::from_int(static_cast<std::int64_t>(u));
    }

    if (v.is_string()) {
        return Id::from_string(v.get<std::string>());
    }

    throw std::runtime_error("rpc: invalid id type");
}


static Error parse_error(const nlohmann::json &e) 
{
    if (!e.is_object()) throw std::runtime_error("rpc: error must be object");

    Error out;
    if (!e.contains("code") || !e["code"].is_number_integer()) {
        throw std::runtime_error("rpc: error.code must be integer");
    }
    out.code = e["code"].get<int>();

    if (!e.contains("message") || !e["message"].is_string()) {
        throw std::runtime_error("rpc: error.message must be string");
    }
    out.message = e["message"].get<std::string>();

    if (e.contains("data")) {
        out.data_json = e["data"].dump();
    }

    return out;
}


Message NlohmannCodec::decode(std::string &&payload) 
{
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(payload.begin(), payload.end());
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("rpc: JSON parse error: ") + e.what());
    }

    if (!j.is_object()) {
        throw std::runtime_error("rpc: message must be object");
    }

    require_jsonrpc_2(j);

    // Request/Notification
    if (j.contains("method")) {
        if (!j["method"].is_string()) {
            throw std::runtime_error("rpc: method must be string");
        }

        const std::string method = j["method"].get<std::string>();
        std::optional<std::string> params;
        if (j.contains("params")) {
            params = j["params"].dump(); // raw JSON value text
        }

        if (j.contains("id")) {
            Request r;
            r.id = parse_id(j["id"]);
            r.method = method;
            r.params_json = std::move(params);
            return r;
        }

        Notification n;
        n.method = method;
        n.params_json = std::move(params);
        return n;
    }

    // Response
    if (!j.contains("id")) {
        throw std::runtime_error("rpc: invalid message (no method and no id)");
    }

    Response r;
    r.id = parse_id(j["id"]);

    const bool has_result = j.contains("result");
    const bool has_error  = j.contains("error");
    if (has_result == has_error) {
        throw std::runtime_error("rpc: response must contain exactly one of result or error");
    }

    if (has_result) {
        r.result_json = j["result"].dump();
    } else {
        r.error = parse_error(j["error"]);
    }

    return r;
}


std::string NlohmannCodec::encode(const Message &msg) 
{
    // note: encoding stays JSON-library-free and consistent across codecs.
    return encode_message_json(msg);
}

} // namespace pcr::rpc



