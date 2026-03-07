#pragma once

#include "pcr/framing/frame_stats.h"

#include "pcr/channel/any_stream.h"
#include "pcr/channel/stream.h"

#include <optional>
#include <string>
#include <string_view>

namespace pcr::framing 
{

// NDJSON: each frame is one line (ending in '\n'), optionally "\r\n".
// read_frame() returns nullopt on clean EOF with no buffered data.
class NdjsonFramer 
{
public:
    explicit NdjsonFramer(pcr::channel::AnyStream &io,
                          std::size_t max_line_bytes = 8 * 1024 * 1024,
                          FrameStats *stats = nullptr);

    std::optional<std::string> read_frame();
    void write_frame(std::string_view line);

    void set_max_line_bytes(std::size_t n) noexcept { 
        max_line_bytes_ = n; 
    }

    void set_stats(FrameStats *stats) noexcept { 
        stats_ = stats; 
    }

private:
    pcr::channel::AnyStream *io_;
    std::size_t max_line_bytes_;
    FrameStats *stats_;
    std::string buf_;
};

} // namespace pcr::framing
