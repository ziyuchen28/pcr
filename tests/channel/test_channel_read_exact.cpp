#include "pcr/channel/socket_stream.h"
#include "pcr/channel/stream.h"

#include <cassert>
#include <iostream>
#include <string>

#include <sys/socket.h>

int main() {
    using namespace pcr::channel;

    int sv[2] = {-1, -1};
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        throw std::runtime_error("socketpair failed");
    }

    SocketStream writer(sv[0], FdOwnership::Owned);
    SocketStream reader(sv[1], FdOwnership::Owned);

    const std::string payload = "hello";
    write_all(writer, payload);
    writer.close_write();

    char buf[16] = {};
    const std::size_t n = read_exact(reader, buf, 10);

    assert(n == payload.size());
    assert(std::string(buf, n) == payload);

    std::cout << "test_channel_read_exact: ok\n";
    return 0;
}
