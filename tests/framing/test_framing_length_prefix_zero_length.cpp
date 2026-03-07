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

    fw.write_frame("");
    writer.close_write();

    auto got = fr.read_frame();
    assert(got);
    assert(got->empty());

    auto eof = fr.read_frame();
    assert(!eof);

    std::cout << "test_framing_length_prefix_zero_length: ok\n";
    return 0;
}
