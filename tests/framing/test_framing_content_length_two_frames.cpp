#include "pcr/framing/content_length_framer.h"

#include "pcr/stream/any_stream.h"
#include "pcr/stream/socket_stream.h"

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

    stream::AnyStream writer{stream::SocketStream(sv[0], stream::FdOwnership::Owned)};
    stream::AnyStream reader{stream::SocketStream(sv[1], stream::FdOwnership::Owned)};

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
