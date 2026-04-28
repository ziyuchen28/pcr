#include "pcr/framing/content_length_framer.h"

#include "pcr/stream/any_stream.h"
#include "pcr/stream/pipe_stream.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#endif

namespace {

// inline void os_close(int fd) noexcept 
// {
// #ifdef _WIN32
//     ::_close(fd);
// #else
//     ::close(fd);
// #endif
// }


// // tiny RAII fd so tests don't leak on early exceptions during make pipes
// struct UniqueFd 
// {
//     int fd = -1;
//     UniqueFd() = default;
//     explicit UniqueFd(int f) : fd(f) {}
//     ~UniqueFd() 
//     { 
//         if (fd >= 0) {
//             os_close(fd);
//         } 
//     }
//
//     UniqueFd(const UniqueFd&) = delete;
//     UniqueFd &operator=(const UniqueFd&) = delete;
//
//     UniqueFd(UniqueFd &&o) noexcept : fd(o.fd) 
//     { 
//         o.fd = -1; 
//     }
//
//     UniqueFd& operator=(UniqueFd &&o) noexcept 
//     {
//         if (this == &o) return *this;
//         if (fd >= 0) {
//             os_close(fd);
//         }
//         fd = o.fd;
//         o.fd = -1;
//         return *this;
//     }
//
//     int release() noexcept 
//     { 
//         int t = fd; 
//         fd = -1; 
//         return t; 
//     }
// };
//

// static void make_pipe(UniqueFd &r, UniqueFd &w) 
// {
//     int fds[2] = {-1, -1};
//     if (::pipe(fds) != 0) {
//         throw std::runtime_error("pipe() failed");
//     }
//     r = UniqueFd(fds[0]);
//     w = UniqueFd(fds[1]);
// }

static void make_pipe(int &r, int &w) 
{
    int fds[2] = {-1, -1};
#ifdef _WIN32
    // binary mode to avoid CRLF translation
    if (::_pipe(fds, 4096, _O_BINARY) != 0) {
        throw std::runtime_error("_pipe failed");
    }
#else
    if (::pipe(fds) != 0) {
        throw std::runtime_error("pipe() failed");
    }
#endif
    r = fds[0];
    w = fds[1];
}

} // namespace


int main() 
{
    using namespace pcr;

    // UniqueFd a_to_b_r, a_to_b_w;
    // UniqueFd b_to_a_r, b_to_a_w;
   
    int fd_a_to_b_r, fd_a_to_b_w;
    int fd_b_to_a_r, fd_b_to_a_w;
    
    make_pipe(fd_a_to_b_r, fd_a_to_b_w);
    make_pipe(fd_b_to_a_r, fd_b_to_a_w);

    stream::AnyStream A{stream::PipeDuplex(
        fd_b_to_a_r,
        fd_a_to_b_w,
        stream::FdOwnership::Owned,
        stream::FdOwnership::Owned
    )};

    stream::AnyStream B{stream::PipeDuplex(
        fd_a_to_b_r,
        fd_b_to_a_w,
        stream::FdOwnership::Owned,
        stream::FdOwnership::Owned
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

