#include "pcr/stream/pipe_stream.h"
#include "pcr/stream/stream.h"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#else
#error "test_stream_pipe_win.cpp is Windows-only"
#endif

namespace {


struct PipePair
{
    int read_end = -1;
    int write_end = -1;
};


PipePair make_pipe()
{
    int fds[2] = {-1, -1};

    // binary mode to avoid CRLF translation
    if (::_pipe(fds, 4096, _O_BINARY) != 0) {
        throw std::runtime_error("_pipe failed");
    }

    return PipePair{fds[0], fds[1]};
}


void test_reader_writer_roundtrip()
{
    using namespace pcr::stream;

    PipePair p = make_pipe();

    PipeReader reader(p.read_end, FdOwnership::Owned);
    PipeWriter writer(p.write_end, FdOwnership::Owned);

    const std::string payload =
        "ping from PipeWriter\n"
        "ping to PipeReader\n";

    write_all(writer, payload);
    writer.close_write();

    const std::string got = read_until_eof(reader);
    assert(got == payload);
}


void test_duplex_both_directions()
{
    using namespace pcr::stream;

    // A writes to ab.write_end, B reads from ab.read_end
    PipePair ab = make_pipe();

    // B writes to ba.write_end, A reads from ba.read_end
    PipePair ba = make_pipe();

    PipeDuplex A(
        ba.read_end,
        ab.write_end,
        FdOwnership::Owned,
        FdOwnership::Owned
    );

    PipeDuplex B(
        ab.read_end,
        ba.write_end,
        FdOwnership::Owned,
        FdOwnership::Owned
    );

    const std::string ping = "ping from A\n";
    write_all(A, ping);
    A.close_write();

    const std::string got_ping = read_until_eof(B);
    assert(got_ping == ping);

    const std::string pong = "pong from B\r\n";
    write_all(B, pong);
    B.close_write();

    const std::string got_pong = read_until_eof(A);
    assert(got_pong == pong);
}

} // namespace

int main()
{
    test_reader_writer_roundtrip();
    test_duplex_both_directions();

    std::cout << "test_stream_pipe_win: ok\n";
    return 0;
}

