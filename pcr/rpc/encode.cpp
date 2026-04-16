#include "encode.h"

#include "id.h"

#include <charconv>
#include <stdexcept>
#include <string_view>

namespace pcr::rpc {

    
namespace {


void append_hex2(std::string& out, unsigned char v) 
{
    constexpr char hex[] = "0123456789abcdef";
    out.push_back(hex[(v >> 4) & 0xF]);
    out.push_back(hex[(v >> 0) & 0xF]);
}


void append_json_string(std::string &out, std::string_view s) 
{
    out.push_back('"');
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                // 0x00 - 0x1F are control chars
                if (c < 0x20) {
                    // unicode prefix
                    out += "\\u00";
                    append_hex2(out, c);
                } else {
                    out.push_back(static_cast<char>(c));
                }
        }
    }
    out.push_back('"');
}


void append_id(std::string &out, const Id &id) 
{
    switch (id.kind) {
        case Id::Kind::Null:
            out += "null";
            return;
        case Id::Kind::Int: {
            // std::to_string is slow
            char buf[32];
            auto [p, ec] = std::to_chars(buf, buf + sizeof(buf), id.i, 10);
            if (ec != std::errc()) throw std::runtime_error("encode_message_json: id to_chars failed");
            out.append(buf, static_cast<std::size_t>(p - buf));
            return;
        }
        case Id::Kind::String:
            append_json_string(out, id.s);
            return;
    }
}


void append_common_prefix(std::string &out) 
{
    out += "{\"jsonrpc\":\"2.0\"";
}


} // namespace


std::string encode_message_json(const Message &msg) 
{
    std::string out;
    out.reserve(512);

    std::visit([&](const auto &m) {
        using T = std::decay_t<decltype(m)>;

        if constexpr (std::is_same_v<T, Request>) {
            append_common_prefix(out);
            out += ",\"id\":";
            append_id(out, m.id);
            out += ",\"method\":";
            append_json_string(out, m.method);
            if (m.params_json.has_value()) {
                out += ",\"params\":";
                out += *m.params_json; // raw JSON value
            }
            out += "}";

        } else if constexpr (std::is_same_v<T, Notification>) {
            append_common_prefix(out);
            out += ",\"method\":";
            append_json_string(out, m.method);
            if (m.params_json.has_value()) {
                out += ",\"params\":";
                out += *m.params_json;
            }
            out += "}";

        } else if constexpr (std::is_same_v<T, Response>) {
            append_common_prefix(out);
            out += ",\"id\":";
            append_id(out, m.id);

            const bool has_err = m.error.has_value();
            const bool has_res = m.result_json.has_value();

            if (has_err == has_res) {
                throw std::runtime_error("encode_message_json: response must have exactly one of result_json or error");
            }

            if (has_err) {
                out += ",\"error\":{";
                out += "\"code\":";
                // code
                {
                    char buf[32];
                    auto [p, ec] = std::to_chars(buf, buf + sizeof(buf), m.error->code, 10);
                    if (ec != std::errc()) throw std::runtime_error("encode_message_json: code to_chars failed");
                    out.append(buf, static_cast<std::size_t>(p - buf));
                }
                out += ",\"message\":";
                append_json_string(out, m.error->message);

                if (m.error->data_json.has_value()) {
                    out += ",\"data\":";
                    out += *m.error->data_json;
                }

                out += "}}";
            } else {
                out += ",\"result\":";
                out += *m.result_json;
                out += "}";
            }
        }
    }, msg);

    return out;
}

} // namespace pcr::rpc
