#include "pcr/stream/pipe_stream.h"
#include "pcr/stream/socket_stream.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

int main() {
    using namespace pcr::stream;

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

    std::cout << "test_stream_invalid_fd: ok\n";
    return 0;
}
