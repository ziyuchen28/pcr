#pragma once

#include "pcr/rpc/message.h"

#include <concepts>
#include <string>

namespace pcr::rpc {

// Decode consumes the frame payload buffer (std::string&&) so future simdjson
// can reserve/pad in-place without extra copies.
template <class C>
concept Codec = requires(C& c, std::string&& payload, const Message& msg) {
    { c.decode(std::move(payload)) } -> std::same_as<Message>;
    { c.encode(msg) } -> std::same_as<std::string>;
};

} // namespace pcr::rpc
