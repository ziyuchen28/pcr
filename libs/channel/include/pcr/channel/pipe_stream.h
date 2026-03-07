#pragma once

#include "pcr/channel/fd_ownership.h"

#include <cstddef>


namespace pcr::channel {


class PipeReader 
{
public:
    explicit PipeReader(int read_fd, FdOwnership ownership = FdOwnership::Owned);
    ~PipeReader() noexcept;
    PipeReader(const PipeReader&) = delete;
    PipeReader &operator=(const PipeReader&) = delete;
    PipeReader(PipeReader &&other) noexcept;
    PipeReader &operator=(PipeReader &&other) noexcept;
    std::size_t read_some(void* dst, std::size_t max_bytes);
    void close_read();
    int fd() const noexcept { return fd_; }
    bool open() const noexcept { return open_; }
private:
    void do_close() noexcept;

    int fd_ = -1;
    bool owned_ = false;
    bool open_ = true;
};


class PipeWriter 
{
public:
    explicit PipeWriter(int write_fd, FdOwnership ownership = FdOwnership::Owned);
    ~PipeWriter() noexcept;

    PipeWriter(const PipeWriter&) = delete;
    PipeWriter &operator=(const PipeWriter&) = delete;

    PipeWriter(PipeWriter &&other) noexcept;
    PipeWriter &operator=(PipeWriter &&other) noexcept;

    std::size_t write_some(const void* src, std::size_t max_bytes);
    void close_write();

    int fd() const noexcept { return fd_; }
    bool open() const noexcept { return open_; }

private:
    void do_close() noexcept;

    int fd_ = -1;
    bool owned_ = false;
    bool open_ = true;
};

// A duplex stream composed of a read-only endpoint and a write-only endpoint.
// This is the natural model for pipes (two different fds).
class PipeDuplex 
{
public:
    PipeDuplex(int read_fd,
               int write_fd,
               FdOwnership read_ownership = FdOwnership::Owned,
               FdOwnership write_ownership = FdOwnership::Owned);

    PipeDuplex(PipeReader reader, PipeWriter writer) noexcept;

    ~PipeDuplex() noexcept = default;

    PipeDuplex(const PipeDuplex&) = delete;
    PipeDuplex& operator=(const PipeDuplex&) = delete;

    PipeDuplex(PipeDuplex&&) noexcept = default;
    PipeDuplex& operator=(PipeDuplex&&) noexcept = default;

    std::size_t read_some(void* dst, std::size_t max_bytes);
    std::size_t write_some(const void* src, std::size_t max_bytes);

    void close_read();
    void close_write();

    int read_fd() const noexcept { return reader_.fd(); }
    int write_fd() const noexcept { return writer_.fd(); }

private:
    PipeReader reader_;
    PipeWriter writer_;
};

} // namespace pcr::channel
