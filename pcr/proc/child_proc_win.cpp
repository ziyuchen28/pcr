#include "pcr/proc/child_proc.h"
#include "pcr/proc/piped_child.h"

#include <chrono>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <io.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace pcr::proc {


namespace {


[[noreturn]] void throw_win32(std::string_view prefix)
{
    const DWORD err = ::GetLastError();
    throw std::runtime_error(std::string(prefix) + ": win32 error " + std::to_string(err));
}

void close_handle_if_open(HANDLE &h) noexcept
{
    if (h != nullptr && h != INVALID_HANDLE_VALUE) {
        ::CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    }
}

std::wstring utf8_to_utf16(std::string_view s)
{
    if (s.empty()) {
        return {};
    }

    const int n = ::multibytetowidechar(
        cp_utf8,
        MB_ERR_INVALID_CHARS,
        s.data(),
        static_cast<int>(s.size()),
        nullptr,
        0);
    if (n <= 0) {
        throw_win32("MultiByteToWideChar(size) failed");
    }

    std::wstring out(static_cast<std::size_t>(n), L'\0');
    const int rc = ::MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        s.data(),
        static_cast<int>(s.size()),
        out.data(),
        n);
    if (rc != n) {
        throw_win32("MultiByteToWideChar(data) failed");
    }

    return out;
}


std::wstring quote_windows_arg(std::wstring_view arg)
{
    if (arg.empty()) {
        return L"\"\"";
    }

    bool needs_quotes = false;
    for (wchar_t ch : arg) {
        if (ch == L' ' || ch == L'\t' || ch == L'"') {
            needs_quotes = true;
            break;
        }
    }

    if (!needs_quotes) {
        return std::wstring(arg);
    }

    std::wstring out;
    out.push_back(L'"');

    std::size_t backslashes = 0;
    for (wchar_t ch : arg) {
        if (ch == L'\\') {
            ++backslashes;
            continue;
        }

        if (ch == L'"') {
            out.append(backslashes * 2 + 1, L'\\');
            out.push_back(L'"');
            backslashes = 0;
            continue;
        }

        if (backslashes > 0) {
            out.append(backslashes, L'\\');
            backslashes = 0;
        }

        out.push_back(ch);
    }

    if (backslashes > 0) {
        out.append(backslashes * 2, L'\\');
    }

    out.push_back(L'"');
    return out;
}


std::wstring build_command_line(const ProcessSpec &spec)
{
    std::wstring cmd = quote_windows_arg(utf8_to_utf16(spec.exe));

    for (const auto& arg : spec.args) {
        cmd.push_back(L' ');
        cmd += quote_windows_arg(utf8_to_utf16(arg));
    }

    return cmd;
}


int fd_from_handle(HANDLE h, int flags)
{
    const int fd = ::_open_osfhandle(reinterpret_cast<intptr_t>(h), flags);
    if (fd < 0) {
        ::CloseHandle(h);
        throw std::runtime_error("_open_osfhandle failed");
    }
    return fd;
}

HANDLE handle_from_fd(int fd)
{
    const intptr_t raw = ::_get_osfhandle(fd);
    if (raw == -1) {
        throw std::runtime_error("_get_osfhandle failed");
    }
    return reinterpret_cast<HANDLE>(raw);
}

struct PipePair {
    HANDLE read_end  = INVALID_HANDLE_VALUE;
    HANDLE write_end = INVALID_HANDLE_VALUE;
};


PipePair make_pipe()
{
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle = TRUE;

    PipePair p;
    if (!::CreatePipe(&p.read_end, &p.write_end, &sa, 0)) {
        throw_win32("CreatePipe failed");
    }
    return p;
}

void close_pipe(PipePair &p) noexcept
{
    close_handle_if_open(p.read_end);
    close_handle_if_open(p.write_end);
}

void make_non_inheritable(HANDLE h)
{
    if (!::SetHandleInformation(h, HANDLE_FLAG_INHERIT, 0)) {
        throw_win32("SetHandleInformation(HANDLE_FLAG_INHERIT=0) failed");
    }
}

ChildProcess from_process_info(PROCESS_INFORMATION pi) noexcept
{
    ChildProcess out;
    out.process_handle_ = pi.hProcess;
    out.pid_ = pi.dwProcessId;

    if (pi.hThread) {
        ::CloseHandle(pi.hThread);
    }
    return out;
}


ChildProcess spawn_with_handles(
    const ProcessSpec &spec,
    HANDLE stdin_h,
    HANDLE stdout_h,
    HANDLE stderr_h)
{
    if (spec.exe.empty()) {
        throw std::invalid_argument("ProcessSpec.exe must not be empty");
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = stdin_h;
    si.hStdOutput = stdout_h;
    si.hStdError = stderr_h;

    PROCESS_INFORMATION pi{};

    std::wstring original_cmd = build_command_line(spec);

    // Wrap it in cmd.exe /c. 
    // path resolution handling 
    std::wstring cmd = L"cmd.exe /c \"" + original_cmd + L"\"";

    std::wstring cwd;
    LPWSTR cwd_ptr = nullptr;
    if (spec.cwd.has_value()) {
        cwd = utf8_to_utf16(*spec.cwd);
        cwd_ptr = cwd.data();
    }

    std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
    cmd_buf.push_back(L'\0');

    if (!::CreateProcessW(
        nullptr,
        cmd_buf.data(),
        nullptr,
        nullptr,
        TRUE,   // inherit std handles
        0,
        nullptr,
        cwd_ptr,
        &si,
        &pi)) 
    {
        throw_win32("CreateProcessW failed");
    }

    return from_process_info(pi);
}


WaitResult decode_exit_code(DWORD code)
{
    WaitResult out;
    out.exited = true;
    out.exit_code = static_cast<int>(code);
    return out;
}


} // namespace

ChildProcess::~ChildProcess()
{
    reap_if_dead();
    if (process_handle_) {
        ::CloseHandle(reinterpret_cast<HANDLE>(process_handle_));
        process_handle_ = nullptr;
    }
}

ChildProcess::ChildProcess(ChildProcess&& other) noexcept
    : process_handle_(other.process_handle_),
      pid_(other.pid_)
{
    other.process_handle_ = nullptr;
    other.pid_ = 0;
}

ChildProcess& ChildProcess::operator=(ChildProcess&& other) noexcept
{
    if (this == &other) return *this;

    if (process_handle_) {
        ::CloseHandle(reinterpret_cast<HANDLE>(process_handle_));
    }

    process_handle_ = other.process_handle_;
    pid_ = other.pid_;
    other.process_handle_ = nullptr;
    other.pid_ = 0;
    return *this;
}

ChildProcess ChildProcess::spawn(const ProcessSpec& spec, const ChildStdioMap& stdio)
{
    HANDLE stdin_h =
        stdio.stdin_fd.has_value() ? handle_from_fd(*stdio.stdin_fd)
                                   : ::GetStdHandle(STD_INPUT_HANDLE);

    HANDLE stdout_h =
        stdio.stdout_fd.has_value() ? handle_from_fd(*stdio.stdout_fd)
                                    : ::GetStdHandle(STD_OUTPUT_HANDLE);

    HANDLE stderr_h =
        stdio.stderr_fd.has_value() ? handle_from_fd(*stdio.stderr_fd)
                                    : ::GetStdHandle(STD_ERROR_HANDLE);

    return spawn_with_handles(spec, stdin_h, stdout_h, stderr_h);
}

void ChildProcess::terminate(int /*signal_number*/)
{
    if (!process_handle_) return;

    // Windows has no SIGTERM/SIGKILL distinction here.
    // TerminateProcess is the low-level forceful stop.
    if (!::TerminateProcess(reinterpret_cast<HANDLE>(process_handle_), 1)) {
        throw_win32("TerminateProcess failed");
    }
}

WaitResult ChildProcess::wait()
{
    if (!process_handle_) {
        return {};
    }

    const DWORD rc =
        ::WaitForSingleObject(reinterpret_cast<HANDLE>(process_handle_), INFINITE);
    if (rc != WAIT_OBJECT_0) {
        throw_win32("WaitForSingleObject(INFINITE) failed");
    }

    DWORD code = 0;
    if (!::GetExitCodeProcess(reinterpret_cast<HANDLE>(process_handle_), &code)) {
        throw_win32("GetExitCodeProcess failed");
    }

    if (process_handle_) {
        ::CloseHandle(reinterpret_cast<HANDLE>(process_handle_));
        process_handle_ = nullptr;
    }
    pid_ = 0;
    return decode_exit_code(code);
}

std::optional<WaitResult> ChildProcess::wait_for(std::chrono::milliseconds timeout)
{
    if (!process_handle_) {
        return WaitResult{};
    }

    const DWORD rc = ::WaitForSingleObject(
        reinterpret_cast<HANDLE>(process_handle_),
        static_cast<DWORD>(timeout.count()));

    if (rc == WAIT_TIMEOUT) {
        return std::nullopt;
    }
    if (rc != WAIT_OBJECT_0) {
        throw_win32("WaitForSingleObject(timeout) failed");
    }

    DWORD code = 0;
    if (!::GetExitCodeProcess(reinterpret_cast<HANDLE>(process_handle_), &code)) {
        throw_win32("GetExitCodeProcess failed");
    }

    if (process_handle_) {
        ::CloseHandle(reinterpret_cast<HANDLE>(process_handle_));
        process_handle_ = nullptr;
    }
    pid_ = 0;
    return decode_exit_code(code);
}

void ChildProcess::reap_if_dead() noexcept
{
    if (!process_handle_) return;

    const DWORD rc =
        ::WaitForSingleObject(reinterpret_cast<HANDLE>(process_handle_), 0);
    if (rc == WAIT_OBJECT_0) {
        ::CloseHandle(reinterpret_cast<HANDLE>(process_handle_));
        process_handle_ = nullptr;
        pid_ = 0;
    }
}

PipedChild::~PipedChild()
{
    close_fds();
}

PipedChild::PipedChild(PipedChild&& other) noexcept
    : process_(std::move(other.process_)),
      parent_write_stdin_(other.parent_write_stdin_),
      parent_read_stdout_(other.parent_read_stdout_),
      parent_read_stderr_(other.parent_read_stderr_)
{
    other.parent_write_stdin_ = -1;
    other.parent_read_stdout_ = -1;
    other.parent_read_stderr_ = -1;
}

PipedChild& PipedChild::operator=(PipedChild&& other) noexcept
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
    PipePair stdin_pipe = make_pipe();   // parent writes, child reads
    PipePair stdout_pipe = make_pipe();  // child writes, parent reads
    PipePair stderr_pipe = make_pipe();  // child writes, parent reads

    try {
        // Parent ends must not be inherited by the child.
        make_non_inheritable(stdin_pipe.write_end);
        make_non_inheritable(stdout_pipe.read_end);
        make_non_inheritable(stderr_pipe.read_end);

        ChildProcess child = spawn_with_handles(
            spec,
            stdin_pipe.read_end,
            stdout_pipe.write_end,
            stderr_pipe.write_end);

        close_handle_if_open(stdin_pipe.read_end);
        close_handle_if_open(stdout_pipe.write_end);
        close_handle_if_open(stderr_pipe.write_end);

        const int parent_stdin_fd =
            fd_from_handle(stdin_pipe.write_end, _O_BINARY | _O_WRONLY);
        stdin_pipe.write_end = INVALID_HANDLE_VALUE;

        const int parent_stdout_fd =
            fd_from_handle(stdout_pipe.read_end, _O_BINARY | _O_RDONLY);
        stdout_pipe.read_end = INVALID_HANDLE_VALUE;

        const int parent_stderr_fd =
            fd_from_handle(stderr_pipe.read_end, _O_BINARY | _O_RDONLY);
        stderr_pipe.read_end = INVALID_HANDLE_VALUE;

        return from_raw(
            std::move(child),
            parent_stdin_fd,
            parent_stdout_fd,
            parent_stderr_fd);
    } catch (...) {
        close_pipe(stdin_pipe);
        close_pipe(stdout_pipe);
        close_pipe(stderr_pipe);
        throw;
    }
}


PipedChild PipedChild::spawn_inherit_stderr(const ProcessSpec &spec)
{
    PipePair stdin_pipe = make_pipe();   // parent writes, child reads
    PipePair stdout_pipe = make_pipe();  // child writes, parent reads

    try {
        make_non_inheritable(stdin_pipe.write_end);
        make_non_inheritable(stdout_pipe.read_end);

        ChildProcess child = spawn_with_handles(
            spec,
            stdin_pipe.read_end,
            stdout_pipe.write_end,
            ::GetStdHandle(STD_ERROR_HANDLE));

        close_handle_if_open(stdin_pipe.read_end);
        close_handle_if_open(stdout_pipe.write_end);

        const int parent_stdin_fd =
            fd_from_handle(stdin_pipe.write_end, _O_BINARY | _O_WRONLY);
        stdin_pipe.write_end = INVALID_HANDLE_VALUE;

        const int parent_stdout_fd =
            fd_from_handle(stdout_pipe.read_end, _O_BINARY | _O_RDONLY);
        stdout_pipe.read_end = INVALID_HANDLE_VALUE;

        return from_raw(
            std::move(child),
            parent_stdin_fd,
            parent_stdout_fd,
            -1);
    } catch (...) {
        close_pipe(stdin_pipe);
        close_pipe(stdout_pipe);
        throw;
    }
}


void PipedChild::close_stdin_write()
{
    if (parent_write_stdin_ >= 0) {
        ::_close(parent_write_stdin_);
        parent_write_stdin_ = -1;
    }
}


WaitResult PipedChild::wait()
{
    return process_.wait();
}


std::optional<WaitResult> PipedChild::wait_for(std::chrono::milliseconds timeout)
{
    return process_.wait_for(timeout);
}


void PipedChild::close_fds() noexcept
{
    if (parent_write_stdin_ >= 0) {
        ::_close(parent_write_stdin_);
        parent_write_stdin_ = -1;
    }
    if (parent_read_stdout_ >= 0) {
        ::_close(parent_read_stdout_);
        parent_read_stdout_ = -1;
    }
    if (parent_read_stderr_ >= 0) {
        ::_close(parent_read_stderr_);
        parent_read_stderr_ = -1;
    }
}

} // namespace pcr::proc


