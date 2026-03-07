#include "pcr/proc/child_proc.h"
#include "pcr/proc/child_stdio.h"
#include "pcr/proc/proc_spec.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    using namespace pcr::proc;

    ProcessSpec spec;
    spec.exe = "sleep";
    spec.args = {"30"};

    ChildStdioMap stdio; // inherit stdio
    auto child = ChildProcess::spawn(spec, stdio);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    child.terminate();

    const WaitResult wr = child.wait();

    assert(wr.signaled || wr.exited);
    if (wr.signaled) {
        assert(wr.term_signal == 15);
    }

    std::cout << "test_proc_terminate: ok\n";
    return 0;
}
