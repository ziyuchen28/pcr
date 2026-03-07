#pragma once

#include "pcr/framing/frame_stats.h"

#include "pcr/channel/any_stream.h"
#include "pcr/channel/stream.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pcr::framing {

// 4-byte big-endian length prefix framing:
//
//   [u32_be length][payload bytes...]
//
// Semantics:
// - read_frame():
//     * returns std::nullopt on clean EOF before starting a new frame
//     * throws on EOF mid-prefix or EOF mid-body
//     * throws if length > max_body_bytes
// - write_frame():
//     * throws if payload.size() > UINT32_MAX
class LengthPrefixFramer 
{
public:
    explicit LengthPrefixFramer(pcr::channel::AnyStream &io,
                                std::size_t max_body_bytes = 64ULL * 1024ULL * 1024ULL,
                                FrameStats *stats = nullptr);

    std::optional<std::string> read_frame();
    void write_frame(std::string_view payload);

    void set_max_body_bytes(std::size_t n) noexcept { 
        max_body_bytes_ = n; 
    }

    void set_stats(FrameStats *stats) noexcept { 
        stats_ = stats; 
    }

private:
    pcr::channel::AnyStream *io_;
    std::size_t max_body_bytes_;
    FrameStats *stats_;
};

} // namespace pcr::framing
