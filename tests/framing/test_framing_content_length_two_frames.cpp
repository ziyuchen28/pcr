#include "pcr/framing/content_length_framer.h"

#include "pcr/channel/any_stream.h"
#include "pcr/channel/socket_stream.h"

#include <cassert>
#include <iostream>

#include <sys/socket.h>

int main() 
{
    using namespace pcr;

    int sv[2] = {-1, -1};
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        throw std::runtime_error("socketpair failed");
    }

    channel::AnyStream writer{channel::SocketStream(sv[0], channel::FdOwnership::Owned)};
    channel::AnyStream reader{channel::SocketStream(sv[1], channel::FdOwnership::Owned)};

    framing::ContentLengthFramer fw(writer);
    framing::ContentLengthFramer fr(reader);

    fw.write_frame("hello");
    fw.write_frame("world");
    writer.close_write(); // signal EOF for this direction

    auto a = fr.read_frame();
    auto b = fr.read_frame();
    auto c = fr.read_frame();

    assert(a.has_value() && *a == "hello");
    assert(b.has_value() && *b == "world");
    assert(!c.has_value());

    std::cout << "test_framing_content_length_two_frames: ok\n";
    return 0;
}
