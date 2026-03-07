#include "pcr/framing/length_prefix_framer.h"

#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace pcr::framing {
namespace {

constexpr std::size_t kPrefixBytes = 4;

static void encode_u32_be(std::uint32_t v, unsigned char out[4]) {
    out[0] = static_cast<unsigned char>((v >> 24) & 0xFF);
    out[1] = static_cast<unsigned char>((v >> 16) & 0xFF);
    out[2] = static_cast<unsigned char>((v >>  8) & 0xFF);
    out[3] = static_cast<unsigned char>((v >>  0) & 0xFF);
}

static std::uint32_t decode_u32_be(const unsigned char in[4]) {
    return (static_cast<std::uint32_t>(in[0]) << 24) |
           (static_cast<std::uint32_t>(in[1]) << 16) |
           (static_cast<std::uint32_t>(in[2]) <<  8) |
           (static_cast<std::uint32_t>(in[3]) <<  0);
}

} // namespace

LengthPrefixFramer::LengthPrefixFramer(pcr::channel::AnyStream& io,
                                       std::size_t max_body_bytes,
                                       FrameStats *stats)
    : io_(&io), max_body_bytes_(max_body_bytes), stats_(stats) {}

std::optional<std::string> LengthPrefixFramer::read_frame() {
    unsigned char prefix[kPrefixBytes] = {0, 0, 0, 0};

    // Read exactly 4 bytes for prefix.
    const std::size_t got_prefix = pcr::channel::read_exact(*io_, prefix, kPrefixBytes);

    if (stats_) {
        stats_->bytes_read += got_prefix;
    }

    if (got_prefix == 0) {
        // Clean EOF before starting a new frame.
        return std::nullopt;
    }

    if (got_prefix != kPrefixBytes) {
        throw std::runtime_error("LengthPrefixFramer: EOF while reading length prefix");
    }

    if (stats_) {
        stats_->header_bytes_read += kPrefixBytes; // treat prefix as "header"/control bytes
    }

    const std::uint32_t len_u32 = decode_u32_be(prefix);
    const std::size_t len = static_cast<std::size_t>(len_u32);

    if (len > max_body_bytes_) {
        throw std::runtime_error("LengthPrefixFramer: frame length exceeds max_body_bytes");
    }

    std::string out;
    out.resize(len);

    if (len > 0) {
        const std::size_t got_body = pcr::channel::read_exact(*io_, out.data(), len);

        if (stats_) {
            stats_->bytes_read += got_body;
        }

        if (got_body != len) {
            throw std::runtime_error("LengthPrefixFramer: EOF while reading frame body");
        }
    }

    if (stats_) {
        stats_->frames_read += 1;
    }

    return out;
}

void LengthPrefixFramer::write_frame(std::string_view payload) {
    if (payload.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        throw std::runtime_error("LengthPrefixFramer: payload too large for u32 length prefix");
    }

    unsigned char prefix[kPrefixBytes];
    encode_u32_be(static_cast<std::uint32_t>(payload.size()), prefix);

    pcr::channel::write_all(*io_, prefix, kPrefixBytes);
    if (!payload.empty()) {
        pcr::channel::write_all(*io_, payload);
    }

    if (stats_) {
        stats_->bytes_written += kPrefixBytes + payload.size();
        stats_->frames_written += 1;
        // (FramingStats currently doesn't have header_bytes_written; we keep it simple.)
    }
}

} // namespace pcr::framing
