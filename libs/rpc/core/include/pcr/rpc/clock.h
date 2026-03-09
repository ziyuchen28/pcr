#pragma once

#include <cstdint>
#include <ctime>

namespace pcr::rpc {

// Monotonic clock in ns.
inline std::uint64_t now_ns() noexcept {
    timespec ts{};
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<std::uint64_t>(ts.tv_sec) * 1000000000ull +
           static_cast<std::uint64_t>(ts.tv_nsec);
}

} // namespace pcr::rpc
