

#include "pcr/proc/piped_child.h"
#include "test_helpers.h"

#include <cassert>
#include <iostream>
#include <string>

int main() 
{
    using namespace pcr::proc;

    ProcessSpec spec;
    spec.exe = "cat";

    auto child = PipedChild::spawn(spec);

    const std::string payload =
        "hello from pcr::proc\n"
        "round-trip via pipes\n";

    write_all_fd(child.stdin_write_fd(), payload);
    child.close_stdin_write();

    const std::string out = read_all_fd(child.stdout_read_fd());
    const std::string err = read_all_fd(child.stderr_read_fd());
    const WaitResult wr = child.wait();

    assert(out == payload);
    assert(err.empty());
    assert(wr.exited);
    assert(wr.exit_code == 0);

    std::cout << "test_proc_piped_cat: ok\n";
    return 0;
}
