#include "pcr/proc/piped_child.h"
#include "test_helpers.h"

#include <cassert>
#include <iostream>
#include <string>

int main() 
{
    using namespace pcr::proc;

    ProcessSpec spec;
    spec.exe = "findstr";
    spec.args = {"^"};

    auto child = PipedChild::spawn(spec);

    const std::string payload = "testing echo from pcr::proc\n";

    write_all_fd(child.stdin_write_fd(), payload);
    child.close_stdin_write();

    const std::string out = read_all_fd(child.stdout_read_fd());
    const std::string err = read_all_fd(child.stderr_read_fd());
    const WaitResult wr = child.wait();

    // handle carriage return for native win tool
    const std::string expected_out = "hello from pcr::proc\r\n";

    assert(out == expected_out);
    assert(err.empty());
    assert(wr.exited);
    assert(wr.exit_code == 0);

    std::cout << "test_proc_piped_win: ok\n";
    return 0;
}
