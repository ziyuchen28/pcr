#include "pcr/proc/child_proc.h"
#include "pcr/proc/child_stdio.h"
#include "pcr/proc/proc_spec.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <fcntl.h>
#include <unistd.h>

namespace {
std::string slurp_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("failed to open " + path);
    std::string s((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return s;
}
}

int main() {
    using namespace pcr::proc;

    char in_tmpl[]  = "/tmp/pcr-proc-in-XXXXXX";
    char out_tmpl[] = "/tmp/pcr-proc-out-XXXXXX";

    int in_fd  = ::mkstemp(in_tmpl);
    int out_fd = ::mkstemp(out_tmpl);
    if (in_fd < 0 || out_fd < 0) {
        throw std::runtime_error("mkstemp failed");
    }

    const std::string input_path  = in_tmpl;
    const std::string output_path = out_tmpl;

    const std::string payload = "generic stdio mapping\nthrough file descriptors\n";
    {
        const ssize_t n = ::write(in_fd, payload.data(), payload.size());
        if (n != static_cast<ssize_t>(payload.size())) {
            throw std::runtime_error("write input file failed");
        }
        ::lseek(in_fd, 0, SEEK_SET);
        ::ftruncate(out_fd, 0);
        ::lseek(out_fd, 0, SEEK_SET);
    }

    int devnull = ::open("/dev/null", O_WRONLY);
    if (devnull < 0) {
        throw std::runtime_error("open /dev/null failed");
    }

    ProcessSpec spec;
    spec.exe = "cat";

    ChildStdioMap stdio;
    stdio.stdin_fd  = in_fd;
    stdio.stdout_fd = out_fd;
    stdio.stderr_fd = devnull;

    auto child = ChildProcess::spawn(spec, stdio);

    // Parent no longer needs these.
    ::close(in_fd);
    ::close(out_fd);
    ::close(devnull);

    const WaitResult wr = child.wait();
    assert(wr.exited);
    assert(wr.exit_code == 0);

    const std::string out = slurp_file(output_path);
    assert(out == payload);

    std::filesystem::remove(input_path);
    std::filesystem::remove(output_path);

    std::cout << "test_proc_generic_redirection: ok\n";
    return 0;
}
