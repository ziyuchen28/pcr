#pragma once

#include "pcr/proc/child_stdio.h"
#include "pcr/proc/proc_spec.h"

#include <sys/types.h>

#include <chrono>
#include <optional>

namespace pcr::proc {

#ifdef _WIN32
using ProcessId = unsigned long;
#else
using ProcessId = pid_t;
#endif

struct WaitResult 
{
    bool exited = false;
    int exit_code = -1;

    bool signaled = false;
    int term_signal = 0;
};


class ChildProcess 
{
public:
    ChildProcess() = default;
    ~ChildProcess();

    ChildProcess(const ChildProcess&) = delete;
    ChildProcess &operator=(const ChildProcess&) = delete;

    ChildProcess(ChildProcess &&other) noexcept;
    ChildProcess &operator=(ChildProcess &&other) noexcept;
    
    // to do: future support - spawn detached process 
    // hence prefer named constructor over constructor for overload extensibility 
    static ChildProcess spawn(const ProcessSpec &spec, const ChildStdioMap &stdio);

    ProcessId pid() const noexcept { 
        return pid_; 
    }

    void terminate(int signal_number = 15); // SIGTERM
    // to do void term_graceful(std::chrono::milliseconds timeout); // SIGTERM -> wait -> SIGKILL
    WaitResult wait();
    std::optional<WaitResult> wait_for(std::chrono::milliseconds timeout);

private:
    static ChildProcess from_pid(ProcessId pid) noexcept;
    void reap_if_dead() noexcept;

    ProcessId pid_ = -1;
#ifdef _WIN32
    void *process_handle_ = nullptr;
#endif

};

} // namespace pcr::proc



