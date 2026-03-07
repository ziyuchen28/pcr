#include "pcr/proc/child_proc.h"
#include "pcr/proc/child_stdio.h"
#include "pcr/proc/proc_spec.h"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>


int main() 
{
    using namespace pcr::proc;

    ProcessSpec spec;
    spec.exe = "pcr-this-command-definitely-does-not-exist";

    ChildStdioMap stdio;

    bool threw = false;

    try {
        auto child = ChildProcess::spawn(spec, stdio);
        (void)child;
    } catch (const std::runtime_error &e) {
        threw = true;
        const std::string msg = e.what();
        std::cout << msg << std::endl;
        assert(msg.find("execvp or applting args failed") != std::string::npos);
    }

    assert(threw);

    std::cout << "test_proc_spawn_fail: ok\n";
    return 0;
}
