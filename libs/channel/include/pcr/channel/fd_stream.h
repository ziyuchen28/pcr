#pragma once

#include <cstddef>

namespace pcr::channel {

enum class FdOwnership 
{
    Borrowed,
    Owned,
};

// A stream backed by POSIX fd
//
// supports:
// - separate read/write fds (pipes)
// - a single full-duplex fd (socketpair, stream sockets)
//
// ownership:
// - Borrowed: destructor will not close the fd(s)
// - Owned: destructor will close the fd(s)
//
// close_read/close_write:
// - for Owned, attempts a half-close (shutdown for sockets, close for pipes)
// - for Borrowed, only marks the wrapper side logically closed
class FdStream 
{
public:
    FdStream(int read_fd,
             int write_fd,
             FdOwnership read_ownership = FdOwnership::Owned,
             FdOwnership write_ownership = FdOwnership::Owned);

    static FdStream from_single_fd(int fd, FdOwnership ownership = FdOwnership::Owned);

    ~FdStream() noexcept;

    FdStream(const FdStream&) = delete;
    FdStream &operator=(const FdStream&) = delete;

    FdStream(FdStream&& other) noexcept;
    FdStream& operator=(FdStream&& other) noexcept;

    std::size_t read_some(void* dst, std::size_t max_bytes);
    std::size_t write_some(const void* src, std::size_t max_bytes);

    void close_read();
    void close_write();

    int read_fd() const noexcept { return read_fd_; }
    int write_fd() const noexcept { return write_fd_; }

    bool read_open() const noexcept { return read_open_; }
    bool write_open() const noexcept { return write_open_; }

private:
    void close_underlying_noexcept() noexcept;
    void maybe_close_same_fd_if_fully_closed_noexcept() noexcept;

    int read_fd_ = -1;
    int write_fd_ = -1;
    bool same_fd_ = false;

    bool own_read_ = false;
    bool own_write_ = false;

    bool read_open_ = true;
    bool write_open_ = true;
};

} // namespace pcr::channel
