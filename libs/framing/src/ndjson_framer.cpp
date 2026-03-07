#include "pcr/framing/ndjson_framer.h"

#include <algorithm>
#include <stdexcept>

namespace pcr::framing {


NdjsonFramer::NdjsonFramer(pcr::channel::AnyStream &io,
                           std::size_t max_line_bytes,
                           FrameStats *stats)
    : io_(&io), max_line_bytes_(max_line_bytes), stats_(stats) {}


std::optional<std::string> NdjsonFramer::read_frame() 
{
    for (;;) {
        const std::size_t nl = buf_.find('\n');
        if (nl != std::string::npos) {
            std::string line = buf_.substr(0, nl);
            buf_.erase(0, nl + 1);

            // Handle \r\n
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (stats_) stats_->frames_read += 1;
            return line;
        }

        if (buf_.size() > max_line_bytes_) {
            throw std::runtime_error("NdjsonFramer: line exceeds max_line_bytes");
        }

        char tmp[4096];
        const std::size_t got = io_->read_some(tmp, sizeof(tmp));
        if (got == 0) {
            // EOF
            if (buf_.empty()) {
                return std::nullopt;
            }
            // Return trailing last line even if no '\n' at EOF.
            std::string last = std::move(buf_);
            buf_.clear();
            if (!last.empty() && last.back() == '\r') {
                last.pop_back();
            }
            if (stats_) stats_->frames_read += 1;
            return last;
        }

        if (stats_) stats_->bytes_read += got;
        buf_.append(tmp, got);
    }
}


void NdjsonFramer::write_frame(std::string_view line) 
{
    pcr::channel::write_all(*io_, line);
    pcr::channel::write_all(*io_, "\n", 1);

    if (stats_) {
        stats_->bytes_written += line.size() + 1;
        stats_->frames_written += 1;
    }
}

} // namespace pcr::framing
