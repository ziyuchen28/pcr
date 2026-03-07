#pragma once

#include <cstdint>

namespace pcr::framing 
{

// optional stats for benchmarking
struct FrameStats 
{
    std::uint64_t bytes_read = 0;
    std::uint64_t bytes_written = 0;

    std::uint64_t frames_read = 0;
    std::uint64_t frames_written = 0;

    std::uint64_t header_bytes_read = 0;
};

} // namespace pcr::framing
