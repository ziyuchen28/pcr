#include "pcr/channel/any_stream.h"
#include "pcr/channel/socket_stream.h"
#include "pcr/channel/stream.h"

#include <cassert>
#include <iostream>
#include <string>

#include <sys/socket.h>

int main() { using namespace pcr::channel;

    int sv[2] = {-1, -1};
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        throw std::runtime_error("socketpair failed");
    }

    AnyStream a{SocketStream(sv[0], FdOwnership::Owned)};
    AnyStream b{SocketStream(sv[1], FdOwnership::Owned)};

    const std::string payload = "hello over AnyStream(SocketStream)";
    const std::string ack = "ack";

    write_all(a, payload);
    a.close_write();

    const std::string got_payload = read_until_eof(b);
    assert(got_payload == payload);

    write_all(b, ack);
    b.close_write();

    const std::string got_ack = read_until_eof(a);
    assert(got_ack == ack);

    std::cout << "test_channel_socketpair: ok\n";
    return 0;
}
