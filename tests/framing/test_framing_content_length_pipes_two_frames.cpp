#include "pcr/framing/content_length_framer.h"

#include "pcr/channel/any_stream.h"
#include "pcr/channel/pipe_stream.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

#include <unistd.h>

namespace {


// tiny RAII fd so tests don't leak on early exceptions
struct UniqueFd 
{
    int fd = -1;
    UniqueFd() = default;
    explicit UniqueFd(int f) : fd(f) {}
    ~UniqueFd() { if (fd >= 0) ::close(fd); }

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    UniqueFd(UniqueFd &&o) noexcept : fd(o.fd) 
    { 
        o.fd = -1; 
    }

    UniqueFd& operator=(UniqueFd &&o) noexcept 
    {
        if (this == &o) return *this;
        if (fd >= 0) ::close(fd);
        fd = o.fd;
        o.fd = -1;
        return *this;
    }

    int release() noexcept 
    { 
        int t = fd; 
        fd = -1; 
        return t; 
    }
};


static void make_pipe(UniqueFd &r, UniqueFd &w) 
{
    int fds[2] = {-1, -1};
    if (::pipe(fds) != 0) {
        throw std::runtime_error("pipe() failed");
    }
    r = UniqueFd(fds[0]);
    w = UniqueFd(fds[1]);
}

} // namespace


int main() 
{
    using namespace pcr;

    // Two unidirectional pipes = full duplex channel (like stdio of a child process)
    UniqueFd a_to_b_r, a_to_b_w;
    UniqueFd b_to_a_r, b_to_a_w;
    make_pipe(a_to_b_r, a_to_b_w);
    make_pipe(b_to_a_r, b_to_a_w);

    // Endpoint A: reads from B→A, writes to A→B
    channel::AnyStream A{channel::PipeDuplex(
        b_to_a_r.release(),
        a_to_b_w.release(),
        channel::FdOwnership::Owned,
        channel::FdOwnership::Owned
    )};

    // Endpoint B: reads from A→B, writes to B→A
    channel::AnyStream B{channel::PipeDuplex(
        a_to_b_r.release(),
        b_to_a_w.release(),
        channel::FdOwnership::Owned,
        channel::FdOwnership::Owned
    )};

    framing::ContentLengthFramer fw(A);
    framing::ContentLengthFramer fr(B);

    fw.write_frame("hello");
    fw.write_frame("world");

    // close A's write side so B sees EOF when looking for the next header.
    A.close_write();

    auto f1 = fr.read_frame();
    auto f2 = fr.read_frame();
    auto eof = fr.read_frame(); // should be 

    assert(f1 && *f1 == "hello");
    assert(f2 && *f2 == "world");
    assert(!eof);

    std::cout << "test_framing_content_length_pipes_two_frames: ok\n";
    return 0;
}
