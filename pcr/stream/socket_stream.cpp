#include "pcr/stream/socket_stream.h"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>

#include <sys/socket.h>
#include <unistd.h>

namespace pcr::stream {

[[noreturn]] static void throw_errno(std::string_view prefix) {
    throw std::runtime_error(std::string(prefix) + ": " + std::strerror(errno));
}

static void close_fd_if_open(int &fd) noexcept 
{
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

static void shutdown_best_effort(int fd, int how) 
{
    if (fd < 0) return;
    if (::shutdown(fd, how) != 0) {
        // ignore if already shut down
        if (errno == ENOTCONN || errno == EINVAL) {
            return;
        }
        throw_errno("shutdown failed");
    }
}

SocketStream::SocketStream(int fd, FdOwnership ownership)
    : fd_(fd),
      owned_(ownership == FdOwnership::Owned) 
{
    if (fd_ < 0) {
        throw std::invalid_argument("SocketStream requires non-negative fd");
    }
}

SocketStream::~SocketStream() noexcept 
{
    close();
}

SocketStream::SocketStream(SocketStream &&other) noexcept
    : fd_(other.fd_),
      owned_(other.owned_),
      read_open_(other.read_open_),
      write_open_(other.write_open_) 
{
    other.fd_ = -1;
    other.owned_ = false;
    other.read_open_ = false;
    other.write_open_ = false;
}

SocketStream &SocketStream::operator=(SocketStream &&other) noexcept 
{
    if (this == &other) return *this;

    close();

    fd_ = other.fd_;
    owned_ = other.owned_;
    read_open_ = other.read_open_;
    write_open_ = other.write_open_;

    other.fd_ = -1;
    other.owned_ = false;
    other.read_open_ = false;
    other.write_open_ = false;

    return *this;
}

std::size_t SocketStream::read_some(void *dst, std::size_t max_bytes) 
{
    if (!read_open_ || fd_ < 0) {
        throw std::logic_error("SocketStream: read_some on closed read side");
    }

    if (max_bytes == 0) {
        return 0;
    }

    for (;;) {
        const ssize_t n = ::read(fd_, dst, max_bytes);
        if (n < 0) {
            if (errno == EINTR) continue;
            throw_errno("read failed");
        }
        return static_cast<std::size_t>(n);
    }
}

std::size_t SocketStream::write_some(const void *src, std::size_t max_bytes) 
{
    if (!write_open_ || fd_ < 0) {
        throw std::logic_error("SocketStream: write_some on closed write side");
    }

    if (max_bytes == 0) {
        return 0;
    }

    for (;;) {
#ifdef MSG_NOSIGNAL
        // avoid SIGPIPE
        const ssize_t n = ::send(fd_, src, max_bytes, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) continue;
            throw_errno("send failed");
        }
        return static_cast<std::size_t>(n);
#else
        const ssize_t n = ::write(fd_, src, max_bytes);
        if (n < 0) {
            if (errno == EINTR) continue;
            throw_errno("write failed");
        }
        return static_cast<std::size_t>(n);
#endif
    }
}

void SocketStream::close_read() 
{
    if (!read_open_) return;
    read_open_ = false;
    if (owned_) {
        shutdown_best_effort(fd_, SHUT_RD);
        // close early if both sides are marked closed to free up fd
        if (fd_ >= 0 && !write_open_) {
            close_fd_if_open(fd_);
        }
    }
}

void SocketStream::close_write() 
{
    if (!write_open_) return;
    write_open_ = false;

    if (owned_) {
        shutdown_best_effort(fd_, SHUT_WR);
        // close early if both sides are marked closed free up fd
        if (fd_ >= 0 && !read_open_) {
            close_fd_if_open(fd_);
        }
    }
}

void SocketStream::close() noexcept 
{
    if (!owned_) {
        fd_ = -1;
        read_open_ = false;
        write_open_ = false;
        return;
    }

    close_fd_if_open(fd_);
    read_open_ = false;
    write_open_ = false;
}


} // namespace pcr::stream

