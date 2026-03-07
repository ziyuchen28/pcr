#pragma once

#include "pcr/framing/frame_stats.h"
#include "pcr/channel/any_stream.h"   
#include "pcr/channel/stream.h"      

#include <optional>
#include <string>
#include <string_view>

namespace pcr::framing 
{

// standard http framing including LSP-style framing:
//   Content-Length: N\r\n
//   <other headers>\r\n
//   \r\n
//   <N bytes body>
//
// read_frame() returns:
// - std::nullopt on clean EOF (no buffered data, stream returns EOF while looking for a new header)
// - throws on protocol errors or EOF mid-header/mid-body.
class ContentLengthFramer 
{
public:
    explicit ContentLengthFramer(pcr::channel::AnyStream &io,
                                 std::size_t max_header_bytes,
                                 std::size_t max_body_bytes,
                                 FrameStats *stats = nullptr);

    explicit ContentLengthFramer(pcr::channel::AnyStream &io,
                                 FrameStats *stats = nullptr);

    std::optional<std::string> read_frame();
    void write_frame(std::string_view payload);

    void set_limits(std::size_t max_header_bytes, 
                    std::size_t max_body_bytes) noexcept 
    { 
        max_header_bytes_ = max_header_bytes; 
        max_body_bytes_ = max_body_bytes;
    }

    void set_stats(FrameStats *stats) noexcept { 
        stats_ = stats; 
    }

private:
    pcr::channel::AnyStream *io_;
    std::string buf_;
    std::size_t max_header_bytes_ = 64 * 1024;
    std::size_t max_body_bytes_ = 64 * 1024 * 1024;
    FrameStats *stats_;
};

} // namespace pcr::framing
