#pragma once

#include <cstdint>
// #include <ctime>
#include <chrono>

namespace pcr::jsonrpc {

// Monotonic clock in ns.
// inline std::uint64_t now_ns() noexcept 
// {
//     timespec ts{};
//     ::clock_gettime(CLOCK_MONOTONIC, &ts);
//     return static_cast<std::uint64_t>(ts.tv_sec) * 1000000000ull +
//            static_cast<std::uint64_t>(ts.tv_nsec);
// }

inline std::uint64_t now_ns() noexcept 
{
    auto now = std::chrono::steady_clock::now();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count()
    );
}

} // namespace pcr::jsonrpc
