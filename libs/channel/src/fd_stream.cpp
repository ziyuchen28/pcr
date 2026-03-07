#include "pcr/channel/fd_stream.h"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>

#include <sys/socket.h>
#include <unistd.h>

namespace pcr::channel {


[[noreturn]] static void throw_errno(std::string_view prefix) 
{
    throw std::runtime_error(std::string(prefix) + ": " + std::strerror(errno));
}

static void close_fd_if_open(int &fd) noexcept 
{
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

// Best-effort shutdown helper.
// For non-sockets, shutdown returns ENOTSOCK.
static void shutdown_noexcept(int fd, int how) noexcept 
{
    if (fd < 0) return;
    if (::shutdown(fd, how) != 0) {
        // Ignore common "not a socket" / "already closed" classes of errors.
        if (errno == ENOTSOCK || errno == ENOTCONN || errno == EINVAL) {
            return;
        }
        // Don't throw in noexcept contexts.
    }
}


FdStream::FdStream(int read_fd,
                   int write_fd,
                   FdOwnership read_ownership,
                   FdOwnership write_ownership)
    : read_fd_(read_fd),
      write_fd_(write_fd),
      same_fd_(read_fd == write_fd),
      own_read_(read_ownership == FdOwnership::Owned),
      own_write_(write_ownership == FdOwnership::Owned) 
{
    if (read_fd < 0 || write_fd < 0) {
        throw std::invalid_argument("FdStream requires non-negative fds");
    }
    if (same_fd_ && own_read_ != own_write_) {
        throw std::invalid_argument("FdStream with a single fd requires matching ownership for read/write");
    }
}


FdStream FdStream::from_single_fd(int fd, FdOwnership ownership) 
{
    return FdStream(fd, fd, ownership, ownership);
}


FdStream::~FdStream() noexcept
{
    close_underlying_noexcept();
}


FdStream::FdStream(FdStream &&other) noexcept
    : read_fd_(other.read_fd_),
      write_fd_(other.write_fd_),
      same_fd_(other.same_fd_),
      own_read_(other.own_read_),
      own_write_(other.own_write_),
      read_open_(other.read_open_),
      write_open_(other.write_open_) 
{
    other.read_fd_ = -1;
    other.write_fd_ = -1;
    other.same_fd_ = false;
    other.own_read_ = false;
    other.own_write_ = false;
    other.read_open_ = false;
    other.write_open_ = false;
}

FdStream &FdStream::operator=(FdStream &&other) noexcept 
{
    if (this == &other) return *this;

    close_underlying_noexcept();

    read_fd_ = other.read_fd_;
    write_fd_ = other.write_fd_;
    same_fd_ = other.same_fd_;
    own_read_ = other.own_read_;
    own_write_ = other.own_write_;
    read_open_ = other.read_open_;
    write_open_ = other.write_open_;

    other.read_fd_ = -1;
    other.write_fd_ = -1;
    other.same_fd_ = false;
    other.own_read_ = false;
    other.own_write_ = false;
    other.read_open_ = false;
    other.write_open_ = false;

    return *this;
}

std::size_t FdStream::read_some(void *dst, std::size_t max_bytes) 
{
    if (!read_open_ || read_fd_ < 0) {
        throw std::logic_error("read_some called when read side is closed");
    }

    if (max_bytes == 0) {
        return 0;
    }

    for (;;) {
        const ssize_t n = ::read(read_fd_, dst, max_bytes);
        if (n < 0) {
            if (errno == EINTR) continue;
            throw_errno("read failed");
        }
        return static_cast<std::size_t>(n); // 0 => EOF
    }
}

std::size_t FdStream::write_some(const void* src, std::size_t max_bytes) 
{
    if (!write_open_ || write_fd_ < 0) {
        throw std::logic_error("write_some called when write side is closed");
    }

    if (max_bytes == 0) {
        return 0;
    }

    for (;;) {
        // NOTE: On Linux, writing to a pipe with no readers raises SIGPIPE by default.
        // Libraries generally cannot change global signal dispositions safely.
        // For socket fds, send(MSG_NOSIGNAL) avoids SIGPIPE.
#ifdef MSG_NOSIGNAL
        const ssize_t n = ::send(write_fd_, src, max_bytes, MSG_NOSIGNAL);
        if (n < 0 && errno == ENOTSOCK) {
            // fall back to write()
        } else if (n < 0) {
            if (errno == EINTR) continue;
            throw_errno("send failed");
        } else {
            return static_cast<std::size_t>(n);
        }
#endif
        const ssize_t n2 = ::write(write_fd_, src, max_bytes);
        if (n2 < 0) {
            if (errno == EINTR) continue;
            throw_errno("write failed");
        }
        return static_cast<std::size_t>(n2);
    }
}


void FdStream::close_read() 
{
    if (!read_open_) return;
    read_open_ = false;

    if (!own_read_) return;

    if (same_fd_) {
        shutdown_noexcept(read_fd_, SHUT_RD);
        maybe_close_same_fd_if_fully_closed_noexcept();
        return;
    }

    close_fd_if_open(read_fd_);
}


void FdStream::close_write() 
{
    if (!write_open_) return;
    write_open_ = false;

    if (!own_write_) return;

    if (same_fd_) {
        shutdown_noexcept(write_fd_, SHUT_WR);
        maybe_close_same_fd_if_fully_closed_noexcept();
        return;
    }

    close_fd_if_open(write_fd_);
}


void FdStream::close_all() noexcept 
{
    if (same_fd_) {
        if (own_read_ && read_fd_ >= 0) {
            ::close(read_fd_);
        }
        read_fd_ = -1;
        write_fd_ = -1;
        read_open_ = false;
        write_open_ = false;
        return;
    }

    if (own_read_) {
        close_fd_if_open(read_fd_);
    } else {
        read_fd_ = -1;
    }

    if (own_write_) {
        close_fd_if_open(write_fd_);
    } else {
        write_fd_ = -1;
    }

    read_open_ = false;
    write_open_ = false;
}


void FdStream::maybe_close_same_fd_if_fully_closed_noexcept() noexcept 
{
    if (!same_fd_) return;
    if (!own_read_) return;
    if (read_fd_ < 0) return;

    if (!read_open_ && !write_open_) {
        ::close(read_fd_);
        read_fd_ = -1;
        write_fd_ = -1;
    }
}

} // namespace pcr::channel
