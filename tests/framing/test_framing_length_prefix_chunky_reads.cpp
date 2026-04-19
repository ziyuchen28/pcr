#include "pcr/framing/length_prefix_framer.h"

#include "pcr/stream/any_stream.h"
#include "pcr/stream/socket_stream.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>

#include <sys/socket.h>

namespace {
struct ChunkyDuplex {
    pcr::stream::SocketStream inner;
    std::size_t cap;

    ChunkyDuplex(int fd, std::size_t cap_)
        : inner(fd, pcr::stream::FdOwnership::Owned), cap(cap_) {}

    ChunkyDuplex(ChunkyDuplex&&) noexcept = default;
    ChunkyDuplex& operator=(ChunkyDuplex&&) noexcept = default;

    ChunkyDuplex(const ChunkyDuplex&) = delete;
    ChunkyDuplex& operator=(const ChunkyDuplex&) = delete;

    std::size_t read_some(void* dst, std::size_t n) {
        return inner.read_some(dst, std::min(n, cap));
    }
    std::size_t write_some(const void* src, std::size_t n) {
        return inner.write_some(src, n);
    }
    void close_read() { inner.close_read(); }
    void close_write() { inner.close_write(); }
};
} // namespace

int main() {
    using namespace pcr;

    int sv[2] = {-1, -1};
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        throw std::runtime_error("socketpair failed");
    }

    stream::AnyStream writer{stream::SocketStream(sv[0], stream::FdOwnership::Owned)};
    stream::AnyStream reader{ChunkyDuplex(sv[1], 2)}; // cap reads to 2 bytes

    framing::LengthPrefixFramer fw(writer);
    framing::LengthPrefixFramer fr(reader);

    const std::string payload(4096, 'x');
    fw.write_frame(payload);
    writer.close_write();

    auto got = fr.read_frame();
    assert(got);
    assert(*got == payload);

    auto eof = fr.read_frame();
    assert(!eof);

    std::cout << "test_framing_length_prefix_chunky_reads: ok\n";
    return 0;
}
