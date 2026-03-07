#include "pcr/framing/length_prefix_framer.h"

#include "pcr/channel/any_stream.h"
#include "pcr/channel/socket_stream.h"
#include "pcr/channel/stream.h"

#include <array>
#include <cassert>
#include <iostream>

#include <sys/socket.h>

static void write_u32_be(pcr::channel::AnyStream& s, std::uint32_t v) {
    unsigned char b[4];
    b[0] = static_cast<unsigned char>((v >> 24) & 0xFF);
    b[1] = static_cast<unsigned char>((v >> 16) & 0xFF);
    b[2] = static_cast<unsigned char>((v >>  8) & 0xFF);
    b[3] = static_cast<unsigned char>((v >>  0) & 0xFF);
    pcr::channel::write_all(s, b, 4);
}

int main() {
    using namespace pcr;

    int sv[2] = {-1, -1};
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        throw std::runtime_error("socketpair failed");
    }

    channel::AnyStream writer{channel::SocketStream(sv[0], channel::FdOwnership::Owned)};
    channel::AnyStream reader{channel::SocketStream(sv[1], channel::FdOwnership::Owned)};

    framing::LengthPrefixFramer fr(reader, /*max_body_bytes=*/8);

    // Send a prefix declaring a 100-byte payload, which exceeds limit 8.
    write_u32_be(writer, 100);
    writer.close_write();

    bool threw = false;
    try {
        (void)fr.read_frame();
    } catch (const std::runtime_error&) {
        threw = true;
    }

    assert(threw);

    std::cout << "test_framing_length_prefix_too_large: ok\n";
    return 0;
}
