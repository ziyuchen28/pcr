#pragma once

#include <concepts>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

namespace pcr::channel {

// read_some returns 0 on EOF
template <class R>
concept Reader = requires(R &r, void *dst, std::size_t n) 
{
    { r.read_some(dst, n) } -> std::same_as<std::size_t>;
    { r.close_read() } -> std::same_as<void>;
};

// write_some must make forward progress for n>0 (return >0) or throw
template <class W>
concept Writer = requires(W &w, const void *src, std::size_t n) 
{
    { w.write_some(src, n) } -> std::same_as<std::size_t>;
    { w.close_write() } -> std::same_as<void>;
};

// supports both reading and writing.
template <class S>
concept DuplexStream = Reader<S> && Writer<S>;

// read exactly n bytes unless EOF arrives first
// returns the number of bytes actually read
template <Reader R>
std::size_t read_exact(R &r, void *dst, std::size_t n) 
{
    auto *out = static_cast<char*>(dst);
    std::size_t off = 0;
    while (off < n) {
        const std::size_t got = r.read_some(out + off, n - off);
        if (got == 0) {
            break; // EOF
        }
        off += got;
    }
    return off;
}

template <Writer W>
void write_all(W &w, const void *src, std::size_t n) 
{
    const auto *in = static_cast<const char*>(src);
    std::size_t off = 0;
    while (off < n) {
        const std::size_t wrote = w.write_some(in + off, n - off);
        if (wrote == 0) {
            throw std::runtime_error("write_some returned 0 while write_all expected forward progress");
        }
        off += wrote;
    }
}

template <Writer W>
void write_all(W &w, std::string_view sv) 
{
    write_all(w, sv.data(), sv.size());
}

// reads until EOF.
template <Reader R>
std::string read_until_eof(R &r, std::size_t chunk_size = 4096) 
{
    if (chunk_size == 0) {
        throw std::invalid_argument("read_until_eof chunk_size must be > 0");
    }
    std::string out;
    std::string buf(chunk_size, '\0');
    for (;;) {
        const std::size_t got = r.read_some(buf.data(), buf.size());
        if (got == 0) {
            break;
        }
        out.append(buf.data(), got);
    }

    return out;
}

} // namespace pcr::channel


