#include "pcr/channel/pipe_stream.h"
#include "pcr/channel/socket_stream.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

int main() {
    using namespace pcr::channel;

    bool threw = false;
    try {
        PipeReader r(-1);
        (void)r;
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);

    threw = false;
    try {
        SocketStream s(-1);
        (void)s;
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);

    std::cout << "test_channel_invalid_fd: ok\n";
    return 0;
}
