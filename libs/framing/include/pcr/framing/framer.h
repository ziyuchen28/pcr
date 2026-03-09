#pragma once

#include <concepts>
#include <optional>
#include <string>
#include <string_view>

namespace pcr::framing {

template <class F>
concept Framer = requires(F& f, std::string_view sv) {
    { f.read_frame() } -> std::same_as<std::optional<std::string>>;
    { f.write_frame(sv) } -> std::same_as<void>;
};

} // namespace pcr::framing
