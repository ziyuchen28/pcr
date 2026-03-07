#include "pcr/framing/length_prefix_framer.h"

#include "pcr/channel/any_stream.h"
#include "pcr/channel/socket_stream.h"

#include <cassert>
#include <iostream>

#include <sys/socket.h>

int main() {
    using namespace pcr;

    int sv[2] = {-1, -1};
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        throw std::runtime_error("socketpair failed");
    }

    channel::AnyStream writer{channel::SocketStream(sv[0], channel::FdOwnership::Owned)};
    channel::AnyStream reader{channel::SocketStream(sv[1], channel::FdOwnership::Owned)};

    framing::LengthPrefixFramer fw(writer);
    framing::LengthPrefixFramer fr(reader);

    fw.write_frame("hello");
    fw.write_frame("world");
    writer.close_write();

    auto a = fr.read_frame();
    auto b = fr.read_frame();
    auto c = fr.read_frame();

    assert(a && *a == "hello");
    assert(b && *b == "world");
    assert(!c);

    std::cout << "test_framing_length_prefix_two_frames: ok\n";
    return 0;
}
