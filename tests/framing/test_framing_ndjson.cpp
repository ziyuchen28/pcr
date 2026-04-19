#include "pcr/framing/ndjson_framer.h"

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

    framing::NdjsonFramer fw(writer);
    framing::NdjsonFramer fr(reader);

    fw.write_frame(R"({"a":1})");
    fw.write_frame(R"({"b":2})");
    writer.close_write();

    auto a = fr.read_frame();
    auto b = fr.read_frame();
    auto c = fr.read_frame();

    assert(a.has_value() && *a == R"({"a":1})");
    assert(b.has_value() && *b == R"({"b":2})");
    assert(!c.has_value());

    std::cout << "test_framing_ndjson: ok\n";
    return 0;
}
