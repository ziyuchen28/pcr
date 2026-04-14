#include "pcr/framing/content_length_framer.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <string_view>

namespace pcr::framing {


static bool is_space(char c) 
{
    return c == ' ' || c == '\t';
}

static std::string_view trim(std::string_view s) 
{
    while (!s.empty() && is_space(s.front())) s.remove_prefix(1);
    while (!s.empty() && is_space(s.back())) s.remove_suffix(1);
    return s;
}

// case-insensitive comparison
// std::lower limited by ascii
static bool ascii_iequals(std::string_view a, std::string_view b) 
{
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const unsigned char ac = static_cast<unsigned char>(a[i]);
        const unsigned char bc = static_cast<unsigned char>(b[i]);
        if (std::tolower(ac) != std::tolower(bc)) return false;
    }
    return true;
}

static std::size_t parse_content_length(std::string_view header_block) 
{
    // header_block is everything before \r\n\r\n (no delimiter).
    // Split by \r\n into lines.
    std::size_t pos = 0;
    constexpr std::string_view content_length = "content-length";
    while (pos < header_block.size()) {
        std::size_t eol = header_block.find("\r\n", pos);
        if (eol == std::string_view::npos) {
            eol = header_block.size();
        }

        const std::string_view line = header_block.substr(pos, eol - pos);
        pos = (eol < header_block.size()) ? (eol + 2) : eol;

        if (line.empty()) continue;

        const std::size_t colon = line.find(':');
        if (colon == std::string_view::npos) continue;

        const std::string_view key = trim(line.substr(0, colon));
        const std::string_view val = trim(line.substr(colon + 1));

        if (!ascii_iequals(key, content_length)) continue;

        std::size_t out = 0;
        const auto *first = val.data();
        const auto *last  = val.data() + val.size();

        auto [ptr, ec] = std::from_chars(first, last, out, 10);
        if (ec != std::errc() || ptr == first) {
            throw std::runtime_error("ContentLengthFramer: invalid Content-Length value");
        }

        return out;
    }

    throw std::runtime_error("ContentLengthFramer: missing Content-Length header");
}


ContentLengthFramer::ContentLengthFramer(pcr::channel::AnyStream &io,
                                         std::size_t max_header,
                                         std::size_t max_body,
                                         FrameStats *stats)
    : io_(&io), 
      max_header_bytes_(max_header), 
      max_body_bytes_(max_body), 
      stats_(stats) 
{}


ContentLengthFramer::ContentLengthFramer(pcr::channel::AnyStream &io,
                                         FrameStats *stats)
    : io_(&io), 
      stats_(stats) 
{}


// to do - don't use std::string::erase, bookkeep a temp pointer
std::optional<std::string> ContentLengthFramer::read_frame() 
{

    constexpr std::string_view delim = "\r\n\r\n";
    // read until complete header block ending with \r\n\r\n
    for (;;) {
        const std::size_t delim_pos = buf_.find(delim);
        // header complete
        if (delim_pos != std::string::npos) {
            const std::string_view header_block(buf_.data(), delim_pos);
            if (stats_) {
                stats_->header_bytes_read += delim_pos + delim.size();
            }

            const std::size_t content_len = parse_content_length(header_block);
            if (content_len > max_body_bytes_) {
                throw std::runtime_error("ContentLengthFramer: body exceeds max_body_bytes");
            }
            // consume header+delim from buffer
            buf_.erase(0, delim_pos + delim.size());
            
            // read body of exactly content_len bytes
            std::string out;
            out.resize(content_len);

            // initial contents already in the buffer
            std::size_t copied = 0;
            if (!buf_.empty()) {
                copied = std::min<std::size_t>(content_len, buf_.size());
                std::memcpy(out.data(), buf_.data(), copied);
                buf_.erase(0, copied);
            }

            std::size_t off = copied;
            while (off < content_len) {
                const std::size_t got = io_->read_some(out.data() + off, content_len - off);
                if (got == 0) {
                    throw std::runtime_error("ContentLengthFramer: EOF while reading body");
                }
                if (stats_) stats_->bytes_read += got;
                off += got;
            }

            if (stats_) stats_->frames_read += 1;
            return out;
        }

        if (buf_.size() > max_header_bytes_) {
            throw std::runtime_error("ContentLengthFramer: header exceeds max_header_bytes");
        }

        char tmp[4096];
        const std::size_t got = io_->read_some(tmp, sizeof(tmp));
        if (got == 0) {
            // EOF
            if (buf_.empty()) {
                return std::nullopt;
            }
            // EOF mid-header -> protocol error
            throw std::runtime_error("ContentLengthFramer: EOF while reading header");
        }

        if (stats_) stats_->bytes_read += got;
        buf_.append(tmp, got);
    }
}


void ContentLengthFramer::write_frame(std::string_view payload) 
{
    // "Content-Length: " (16 chars) + up to 20 digits + "\r\n\r\n" (4)
    // std::array<char, 64> hdr{};
    char hdr[64];
    char *p = hdr;

    constexpr std::string_view prefix = "Content-Length: ";
    std::memcpy(p, prefix.data(), prefix.size());
    p += prefix.size();

    char numbuf[32];
    auto [num_end, ec] = std::to_chars(numbuf, numbuf + sizeof(numbuf), payload.size(), 10);
    if (ec != std::errc()) {
        throw std::runtime_error("ContentLengthFramer: failed to format Content-Length");
    }
    const std::size_t numlen = static_cast<std::size_t>(num_end - numbuf);
    std::memcpy(p, numbuf, numlen);
    p += numlen;

    constexpr std::string_view suffix = "\r\n\r\n";
    std::memcpy(p, suffix.data(), suffix.size());
    p += suffix.size();

    const std::size_t hdr_len = static_cast<std::size_t>(p - hdr);

    pcr::channel::write_all(*io_, hdr, hdr_len);
    pcr::channel::write_all(*io_, payload);

    if (stats_) {
        stats_->bytes_written += hdr_len + payload.size();
        stats_->frames_written += 1;
    }
}


} // namespace pcr::framing
