#pragma once

#include "pcr/channel/stream.h"

#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace pcr::channel {


// with (SBO) optimization
class AnyStream 
{
public:
    AnyStream() noexcept = default;

    AnyStream(const AnyStream&) = delete;
    AnyStream &operator=(const AnyStream&) = delete;

    AnyStream(AnyStream &&other) noexcept 
    {
        if (other.vtbl_) {
            other.vtbl_->move(*this, other);
        }
    }

    AnyStream &operator=(AnyStream &&other) noexcept 
    {
        if (this == &other) return *this;
        reset();
        if (other.vtbl_) {
            other.vtbl_->move(*this, other);
        }
        return *this;
    }

    ~AnyStream() noexcept 
    {
        reset();
    }

    template <class S>
    requires DuplexStream<S> && std::is_nothrow_move_constructible_v<S>
    AnyStream(S s) 
    {
        emplace<S>(std::move(s));
    }

    template <class S, class ...Args>
    requires DuplexStream<S> && std::is_nothrow_move_constructible_v<S>
    S &emplace(Args &&...args) 
    {
        reset();
        using M = Model<S>;

        if constexpr (fits_sbo<M>()) {
            obj_ = ::new (static_cast<void*>(storage_)) M(EmptyTag {}, std::forward<Args>(args)...);
            heap_ = false;
        } else {
            obj_ = new M(EmptyTag {}, std::forward<Args>(args)...);
            heap_ = true;
        }

        vtbl_ = &M::vtbl;
        return static_cast<M*>(obj_)->stream;
    }

    void reset() noexcept 
    {
        if (vtbl_) {
            vtbl_->destroy(*this);
        }
    }

    explicit operator bool() const noexcept 
    {
        return vtbl_ != nullptr;
    }

    std::size_t read_some(void *dst, std::size_t max_bytes) 
    {
        if (!vtbl_) throw std::logic_error("AnyStream: read_some on empty");
        return vtbl_->read_some(obj_, dst, max_bytes);
    }

    std::size_t write_some(const void* src, std::size_t max_bytes) {
        if (!vtbl_) throw std::logic_error("AnyStream: write_some on empty");
        return vtbl_->write_some(obj_, src, max_bytes);
    }

    void close_read() 
    {
        if (!vtbl_) return;
        vtbl_->close_read(obj_);
    }

    void close_write() 
    {
        if (!vtbl_) return;
        vtbl_->close_write(obj_);
    }


private:

    // vtbl_: 8, obj_: 8, heap_: 1, padding : 7, alignas(std::max_align_t) -> 32 
    // +96 = 128, two cache lines   
    static constexpr std::size_t k_sbo_size = 96;
    // avoid universal ref deduction confusion
    struct EmptyTag {};

    struct VTable 
    {
        std::size_t (*read_some)(void*, void*, std::size_t);
        std::size_t (*write_some)(void*, const void*, std::size_t);
        void (*close_read)(void*);
        void (*close_write)(void*);

        void (*destroy)(AnyStream&) noexcept;
        void (*move)(AnyStream &dst, AnyStream &src) noexcept;
    };

    template <class M>
    static consteval bool fits_sbo() 
    {
        return sizeof(M) <= k_sbo_size && alignof(M) <= alignof(std::max_align_t);
    }

    template <class S>
    struct Model 
    {
        S stream;

        template <class ...Args>
        explicit Model(EmptyTag, Args &&...args) noexcept
            : stream(std::forward<Args>(args)...) {}

        static std::size_t read_some(void *self, void *dst, std::size_t n) 
        {
            return static_cast<Model*>(self)->stream.read_some(dst, n);
        }

        static std::size_t write_some(void *self, const void *src, std::size_t n) 
        {
            return static_cast<Model*>(self)->stream.write_some(src, n);
        }

        static void close_read(void *self) 
        {
            static_cast<Model*>(self)->stream.close_read();
        }

        static void close_write(void *self) 
        {
            static_cast<Model*>(self)->stream.close_write();
        }

        static void destroy(AnyStream &a) noexcept 
        {
            if (!a.vtbl_) return;
            if (a.heap_) {
                delete static_cast<Model*>(a.obj_);
            } else {
                static_cast<Model*>(a.obj_)->~Model();
            }
            a.vtbl_ = nullptr;
            a.obj_ = nullptr;
            a.heap_ = false;
        }

        static void move(AnyStream &dst, AnyStream &src) noexcept 
        {
            dst.vtbl_ = src.vtbl_;
            if (src.heap_) {
                dst.obj_ = src.obj_;
                dst.heap_ = true;
                src.vtbl_ = nullptr;
                src.obj_ = nullptr;
                src.heap_ = false;
                return;
            }

            // safty check
            static_assert(std::is_nothrow_move_constructible_v<S>,
                          "AnyStream requires nothrow-move stream types");

            auto *src_m = static_cast<Model*>(src.obj_);
            auto *dst_m = ::new (static_cast<void*>(dst.storage_)) Model(EmptyTag {}, std::move(src_m->stream));
            dst.obj_ = dst_m;
            dst.heap_ = false;

            src_m->~Model();
            src.vtbl_ = nullptr;
            src.obj_ = nullptr;
            src.heap_ = false;
        }

        // virtual pointers
        inline static const VTable vtbl{
            &Model::read_some,
            &Model::write_some,
            &Model::close_read,
            &Model::close_write,
            &Model::destroy,
            &Model::move,
        };
    };

    const VTable *vtbl_ = nullptr;
    void *obj_ = nullptr;
    bool heap_ = false;

    alignas(std::max_align_t) unsigned char storage_[k_sbo_size] = {};
};

} // namespace pcr::channel

