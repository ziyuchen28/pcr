#include "child_proc.h"
#include "piped_child.h"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <chrono>
#include <optional>
#include <thread>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace pcr::proc {

namespace {


[[noreturn]] void throw_errno(std::string_view prefix) 
{
    throw std::runtime_error(std::string(prefix) + ": " + std::strerror(errno));
}


WaitResult decode_wait_status(int status)
{
    WaitResult out;
    if (WIFEXITED(status)) {
        out.exited = true;
        out.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        out.signaled = true;
        out.term_signal = WTERMSIG(status);
    }
    return out;
}


void close_fd_if_open(int &fd) noexcept 
{
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

// posix trick to tell parent proc exec succeeded or failed
void set_cloexec(int fd) 
{
    const int flags = ::fcntl(fd, F_GETFD);
    if (flags < 0) {
        throw_errno("fcntl(F_GETFD) failed");
    }
    if (::fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0) {
        throw_errno("fcntl(F_SETFD, FD_CLOEXEC) failed");
    }
}

struct PipePair 
{
    int read_end = -1;
    int write_end = -1;
};

PipePair make_pipe() 
{
    int fds[2] = {-1, -1};
    if (::pipe(fds) != 0) {
        throw_errno("pipe failed");
    }
    set_cloexec(fds[0]);
    set_cloexec(fds[1]);
    return PipePair{fds[0], fds[1]};
}

void close_pipe(PipePair &p) noexcept 
{
    close_fd_if_open(p.read_end);
    close_fd_if_open(p.write_end);
}

std::vector<char*> build_argv(const ProcessSpec &spec) 
{
    std::vector<char*> argv;
    argv.reserve(spec.args.size() + 2);

    argv.push_back(const_cast<char*>(spec.exe.c_str()));
    for (const auto &arg : spec.args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    return argv;
}

void apply_child_setup(const ProcessSpec &spec) 
{
    if (spec.cwd.has_value()) {
        if (::chdir(spec.cwd->c_str()) != 0) {
            throw_errno("chdir failed");
        }
    }
    for (const auto &kv : spec.env_overrides) {
        if (::setenv(kv.key.c_str(), kv.value.c_str(), 1) != 0) {
            throw_errno("setenv failed");
        }
    }
}

void write_errno_and_exit(int err_pipe_fd, int err) noexcept 
{
    (void)::write(err_pipe_fd, &err, sizeof(err));
    // _exit instead of exit to avoid flusing parent's stdio buffer
    _exit(127);
}


void remap_child_stdio(const ChildStdioMap &stdio, int err_pipe_fd) noexcept 
{
    if (stdio.stdin_fd.has_value()) {
        if (::dup2(*stdio.stdin_fd, STDIN_FILENO) < 0) {
            write_errno_and_exit(err_pipe_fd, errno);
        }
    }

    if (stdio.stdout_fd.has_value()) {
        if (::dup2(*stdio.stdout_fd, STDOUT_FILENO) < 0) {
            write_errno_and_exit(err_pipe_fd, errno);
        }
    }

    if (stdio.stderr_fd.has_value()) {
        if (::dup2(*stdio.stderr_fd, STDERR_FILENO) < 0) {
            write_errno_and_exit(err_pipe_fd, errno);
        }
    }

    // edge case: some of stdin_fd, stdout_fd, stderr_fd are same fd
    // double close -> EBADF
    int seen[3] = {-1, -1, -1};
    int count = 0;

    auto maybe_track = [&](const std::optional<int> &fd_opt) {
        if (!fd_opt.has_value()) return;
        const int fd = *fd_opt;
        // edge case: std fd wrongly passed
        if (fd >= 0 && fd > STDERR_FILENO) {
            for (int i = 0; i < count; ++i) {
                if (seen[i] == fd) return;
            }
            seen[count++] = fd;
        }
    };

    // other fds are closed through FD_CLOEXEC 
    maybe_track(stdio.stdin_fd);
    maybe_track(stdio.stdout_fd);
    maybe_track(stdio.stderr_fd);

    for (int i = 0; i < count; ++i) {
        ::close(seen[i]);
    }
}


int read_exec_status_or_success(int fd) 
{
    int child_errno = 0;
    std::size_t got = 0;
    char *dst = reinterpret_cast<char*>(&child_errno);

    while (got < sizeof(child_errno)) {
        const ssize_t n = ::read(fd, dst + got, sizeof(child_errno) - got);
        if (n == 0) {
            break; // success path: fd was closed automatically on exec
        }
        if (n < 0) {
            if (errno == EINTR) continue;
            throw_errno("read(exec-status-pipe) failed");
        }
        got += static_cast<std::size_t>(n);
    }

    // exec success
    if (got == 0) {
        return 0; 
    }

    if (got != sizeof(child_errno)) {
        throw std::runtime_error("short read from exec-status pipe");
    }

    return child_errno;
}

} // namespace



/* ChildProcess */ 

// only destroys the handle
ChildProcess::~ChildProcess() 
{
    reap_if_dead();
}

ChildProcess::ChildProcess(ChildProcess &&other) noexcept
    : pid_(other.pid_) 
{
    other.pid_ = -1;
}

ChildProcess &ChildProcess::operator=(ChildProcess &&other) noexcept 
{
    if (this == &other) return *this;

    reap_if_dead();

    pid_ = other.pid_;
    other.pid_ = -1;
    return *this;
}

ChildProcess ChildProcess::from_pid(pid_t pid) noexcept 
{
    ChildProcess p;
    p.pid_ = pid;
    return p;
}


ChildProcess ChildProcess::spawn(const ProcessSpec &spec, const ChildStdioMap &stdio) 
{
    if (spec.exe.empty()) {
        throw std::invalid_argument("ProcessSpec.exe must not be empty");
    }

    PipePair exec_status_pipe = make_pipe();
    // child writes errno on failure
    // parent reads
    // close-on-exec set in make_pipe, success -> EOF in parent

    const pid_t pid = ::fork();
    if (pid < 0) {
        close_pipe(exec_status_pipe);
        throw_errno("fork failed");
    }

    if (pid == 0) {
        // child
        close_fd_if_open(exec_status_pipe.read_end);
        // write errno to exec_status_pipe upon dup2 failure
        remap_child_stdio(stdio, exec_status_pipe.write_end);
        try {
            apply_child_setup(spec);
            auto argv = build_argv(spec);
            ::execvp(spec.exe.c_str(), argv.data());
        } catch (...) {
            // fall through
        }
        // if apply_child_setup or build_arg fails, errno is 0, need to flag it manually
        const int e = (errno != 0) ? errno : EINVAL;
        write_errno_and_exit(exec_status_pipe.write_end, e);
    }

    // parent
    close_fd_if_open(exec_status_pipe.write_end);

    int child_errno = 0;
    try {
        child_errno = read_exec_status_or_success(exec_status_pipe.read_end);
    } catch (...) {
        close_fd_if_open(exec_status_pipe.read_end);
        throw;
    }

    close_fd_if_open(exec_status_pipe.read_end);

    if (child_errno != 0) {
        int status = 0;
        (void)::waitpid(pid, &status, 0);
        errno = child_errno;
        throw_errno("execvp or applting args failed");
    }

    return from_pid(pid);
}


void ChildProcess::terminate(int signal_number) 
{
    if (pid_ <= 0) return;
    if (::kill(pid_, signal_number) != 0) {
        if (errno != ESRCH) {
            throw_errno("kill failed");
        }
    }
}

WaitResult ChildProcess::wait() 
{
    if (pid_ <= 0) {
        return {};
    }

    int status = 0;
    pid_t rc = -1;
    do {
        // blocking
        rc = ::waitpid(pid_, &status, 0);
    } while (rc < 0 && errno == EINTR);

    if (rc < 0) {
        throw_errno("waitpid failed");
    }

    // WaitResult out;
    // if (WIFEXITED(status)) {
    //     out.exited = true;
    //     out.exit_code = WEXITSTATUS(status);
    // } else if (WIFSIGNALED(status)) {
    //     out.signaled = true;
    //     out.term_signal = WTERMSIG(status);
    // }

    pid_ = -1;
    return decode_wait_status(status);
}


std::optional<WaitResult> ChildProcess::wait_for(std::chrono::milliseconds timeout)
{
    if (pid_ <= 0) {
        return WaitResult{};
    }

    const auto deadline = std::chrono::steady_clock::now() + timeout;

    for (;;) {
        int status = 0;
        const pid_t rc = ::waitpid(pid_, &status, WNOHANG);

        if (rc == pid_) {
            pid_ = -1;
            return decode_wait_status(status);
        }
        if (rc == 0) {
            if (std::chrono::steady_clock::now() >= deadline) {
                return std::nullopt;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (errno == EINTR) {
            continue;
        }

        throw_errno("waitpid(WNOHANG) failed");
    }
}


void ChildProcess::reap_if_dead() noexcept 
{
    if (pid_ <= 0) return;

    int status = 0;
    const pid_t rc = ::waitpid(pid_, &status, WNOHANG);
    if (rc == pid_) {
        pid_ = -1;
    }
}


/* PipedChild */ 

PipedChild::~PipedChild() {
    close_fds();
}


PipedChild::PipedChild(PipedChild &&other) noexcept
    : process_(std::move(other.process_)),
      parent_write_stdin_(other.parent_write_stdin_),
      parent_read_stdout_(other.parent_read_stdout_),
      parent_read_stderr_(other.parent_read_stderr_) 
{
    other.parent_write_stdin_ = -1;
    other.parent_read_stdout_ = -1;
    other.parent_read_stderr_ = -1;
}


PipedChild &PipedChild::operator=(PipedChild &&other) noexcept 
{
    if (this == &other) return *this;

    close_fds();

    process_ = std::move(other.process_);
    parent_write_stdin_ = other.parent_write_stdin_;
    parent_read_stdout_ = other.parent_read_stdout_;
    parent_read_stderr_ = other.parent_read_stderr_;

    other.parent_write_stdin_ = -1;
    other.parent_read_stdout_ = -1;
    other.parent_read_stderr_ = -1;

    return *this;
}

// sink func - takes ownershipt of ChildProcess hence two moves
PipedChild PipedChild::from_raw(
    ChildProcess proc, 
    int parent_write_stdin, 
    int parent_read_stdout, 
    int parent_read_stderr) noexcept 
{
    PipedChild out;
    out.process_ = std::move(proc);
    out.parent_write_stdin_ = parent_write_stdin;
    out.parent_read_stdout_ = parent_read_stdout;
    out.parent_read_stderr_ = parent_read_stderr;
    return out;
}


PipedChild PipedChild::spawn(const ProcessSpec &spec) 
{
    // child reads from it's stdin
    PipePair stdin_pipe = make_pipe();
    // child writes to it's stdout
    PipePair stdout_pipe = make_pipe();
    // child writes to it's stderr
    PipePair stderr_pipe = make_pipe();

    try {
        ChildStdioMap stdio;
        stdio.stdin_fd  = stdin_pipe.read_end;
        stdio.stdout_fd = stdout_pipe.write_end;
        stdio.stderr_fd = stderr_pipe.write_end;

        ChildProcess child = ChildProcess::spawn(spec, stdio);
        // Parent keeps:
        //   stdin write end
        //   stdout read end
        //   stderr read end
        close_fd_if_open(stdin_pipe.read_end);
        close_fd_if_open(stdout_pipe.write_end);
        close_fd_if_open(stderr_pipe.write_end);

        return from_raw(
            std::move(child),
            stdin_pipe.write_end,
            stdout_pipe.read_end,
            stderr_pipe.read_end
        );
    } catch (...) {
        close_pipe(stdin_pipe);
        close_pipe(stdout_pipe);
        close_pipe(stderr_pipe);
        throw;
    }
}


PipedChild PipedChild::spawn_inherit_stderr(const ProcessSpec &spec)
{
    // child reads from stdin pipe
    PipePair stdin_pipe = make_pipe();
    // child writes protocol output to stdout pipe
    PipePair stdout_pipe = make_pipe();

    try {
        ChildStdioMap stdio;
        stdio.stdin_fd  = stdin_pipe.read_end;
        stdio.stdout_fd = stdout_pipe.write_end;

        // The child inherits parent's STDERR_FILENO.
        ChildProcess child = ChildProcess::spawn(spec, stdio);

        // Parent keeps:
        //   stdin write end
        //   stdout read end
        close_fd_if_open(stdin_pipe.read_end);
        close_fd_if_open(stdout_pipe.write_end);

        return from_raw(
            std::move(child),
            stdin_pipe.write_end,
            stdout_pipe.read_end,
            -1
        );
    } catch (...) {
        close_pipe(stdin_pipe);
        close_pipe(stdout_pipe);
        throw;
    }
}


void PipedChild::close_stdin_write() 
{
    close_fd_if_open(parent_write_stdin_);
}


WaitResult PipedChild::wait() 
{
    return process_.wait();
}

void PipedChild::close_fds() noexcept 
{
    close_fd_if_open(parent_write_stdin_);
    close_fd_if_open(parent_read_stdout_);
    close_fd_if_open(parent_read_stderr_);
}

std::optional<WaitResult> PipedChild::wait_for(std::chrono::milliseconds timeout)
{
    return process_.wait_for(timeout);
}


void PipedChild::terminate(int signal_number)
{
    process_.terminate(signal_number);
}


} // namespace pcr::proc


