#include "pcr/framing/length_prefix_framer.h"

#include "pcr/stream/any_stream.h"
#include "pcr/stream/socket_stream.h"

#include <cassert>
#include <iostream>

#include <sys/socket.h>

int main() {
    using namespace pcr;

    int sv[2] = {-1, -1};
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        throw std::runtime_error("socketpair failed");
    }

    stream::AnyStream writer{stream::SocketStream(sv[0], stream::FdOwnership::Owned)};
    stream::AnyStream reader{stream::SocketStream(sv[1], stream::FdOwnership::Owned)};

    framing::LengthPrefixFramer fr(reader);

    // Send only 2 bytes of the 4-byte prefix, then EOF.
    unsigned char partial[2] = {0x00, 0x00};
    pcr::stream::write_all(writer, partial, 2);
    writer.close_write();

    bool threw = false;
    try {
        (void)fr.read_frame();
    } catch (const std::runtime_error&) {
        threw = true;
    }

    assert(threw);

    std::cout << "test_framing_length_prefix_eof_mid_prefix: ok\n";
    return 0;
}
