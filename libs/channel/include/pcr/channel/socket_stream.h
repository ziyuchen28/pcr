#pragma once
#include "pcr/channel/fd_ownership.h"

#include <cstddef>

namespace pcr::channel {

/* duplex stream socket wrapper (works for AF_UNIX SOCK_STREAM, TCP, etc.)
 - close_read/close_write perform shutdown(SHUT_RD/SHUT_WR) when owned
 - For Borrowed, close_* only marks the wrapper side logically closed
 - destructor closes the fd if owned */
class SocketStream 
{
public:
    explicit SocketStream(int fd, FdOwnership ownership = FdOwnership::Owned);
    ~SocketStream() noexcept;

    SocketStream(const SocketStream&) = delete;
    SocketStream &operator=(const SocketStream&) = delete;

    SocketStream(SocketStream &&other) noexcept;
    SocketStream &operator=(SocketStream &&other) noexcept;

    std::size_t read_some(void *dst, std::size_t max_bytes);
    std::size_t write_some(const void *src, std::size_t max_bytes);

    void close_read();
    void close_write();

    int fd() const noexcept { return fd_; }
    bool read_open() const noexcept { return read_open_; }
    bool write_open() const noexcept { return write_open_; }

private:
    void close() noexcept;

    int fd_ = -1;
    bool owned_ = false;
    bool read_open_ = true;
    bool write_open_ = true;
};

} // namespace pcr::channel


