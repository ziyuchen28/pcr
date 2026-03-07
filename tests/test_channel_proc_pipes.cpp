#include "pcr/channel/pipe_stream.h"
#include "pcr/channel/stream.h"
#include "pcr/proc/piped_child.h"

#include <cassert>
#include <iostream>
#include <string>

#include <unistd.h>

static std::string read_all_fd(int fd) 
{
    std::string out;
    char buf[4096];
    for (;;) {
        const ssize_t n = ::read(fd, buf, sizeof(buf));
        if (n == 0) break;
        if (n < 0) continue;
        out.append(buf, static_cast<std::size_t>(n));
    }
    return out;
}

int main() 
{
    using namespace pcr;

    proc::ProcessSpec spec;
    spec.exe = "cat";

    auto child = proc::PipedChild::spawn(spec);

    // Borrow the pipe fds; PipedChild owns and closes them.
    channel::PipeDuplex ch(
        child.stdout_read_fd(),
        child.stdin_write_fd(),
        channel::FdOwnership::Borrowed,
        channel::FdOwnership::Borrowed
    );

    const std::string payload =
        "hello from pcr::channel PipeDuplex over proc pipes\n"
        "round-trip through cat\n";

    channel::write_all(ch, payload);

    // Borrowed: this does not close underlying write fd.
    ch.close_write();

    // Actual EOF for cat:
    child.close_stdin_write();

    const std::string out = channel::read_until_eof(ch);
    const std::string err = read_all_fd(child.stderr_read_fd());
    const proc::WaitResult wr = child.wait();

    assert(out == payload);
    assert(err.empty());
    assert(wr.exited);
    assert(wr.exit_code == 0);

    std::cout << "test_channel_proc_pipes: ok\n";
    return 0;
}
