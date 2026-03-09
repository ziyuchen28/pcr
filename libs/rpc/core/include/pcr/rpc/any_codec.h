#pragma once

#include "pcr/rpc/codec.h"

#include <cstddef>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace pcr::rpc {

// Move-only type-erased Codec with SBO.
class AnyCodec {
public:
    AnyCodec() noexcept = default;

    AnyCodec(const AnyCodec&) = delete;
    AnyCodec& operator=(const AnyCodec&) = delete;

    AnyCodec(AnyCodec&& other) noexcept {
        if (other.vtbl_) other.vtbl_->move(*this, other);
    }

    AnyCodec& operator=(AnyCodec&& other) noexcept {
        if (this == &other) return *this;
        reset();
        if (other.vtbl_) other.vtbl_->move(*this, other);
        return *this;
    }

    ~AnyCodec() noexcept { reset(); }

    template <class C>
        requires Codec<C>
    AnyCodec(C c) {
        emplace<C>(std::move(c));
    }

    template <class C, class... Args>
        requires Codec<C>
    C& emplace(Args&&... args) {
        reset();
        using M = Model<C>;

        constexpr bool can_inline =
            fits_sbo<M>() && std::is_nothrow_move_constructible_v<C>;

        if constexpr (can_inline) {
            obj_ = ::new (static_cast<void*>(storage_)) M(std::in_place, std::forward<Args>(args)...);
            heap_ = false;
        } else {
            obj_ = new M(std::in_place, std::forward<Args>(args)...);
            heap_ = true;
        }

        vtbl_ = &M::vtbl;
        return static_cast<M*>(obj_)->codec;
    }

    void reset() noexcept {
        if (vtbl_) vtbl_->destroy(*this);
    }

    explicit operator bool() const noexcept { return vtbl_ != nullptr; }

    Message decode(std::string&& payload) {
        if (!vtbl_) throw std::logic_error("AnyCodec: decode on empty");
        return vtbl_->decode(obj_, std::move(payload));
    }

    std::string encode(const Message& msg) {
        if (!vtbl_) throw std::logic_error("AnyCodec: encode on empty");
        return vtbl_->encode(obj_, msg);
    }

private:
    static constexpr std::size_t kSboSize = 128;

    struct VTable {
        Message (*decode)(void*, std::string&&);
        std::string (*encode)(void*, const Message&);

        void (*destroy)(AnyCodec&) noexcept;
        void (*move)(AnyCodec& dst, AnyCodec& src) noexcept;
    };

    template <class M>
    static consteval bool fits_sbo() {
        return sizeof(M) <= kSboSize && alignof(M) <= alignof(std::max_align_t);
    }

    template <class C>
    struct Model {
        C codec;

        template <class... Args>
        explicit Model(std::in_place_t, Args&&... args)
            : codec(std::forward<Args>(args)...) {}

        static Message decode(void* self, std::string&& payload) {
            return static_cast<Model*>(self)->codec.decode(std::move(payload));
        }

        static std::string encode(void* self, const Message& msg) {
            return static_cast<Model*>(self)->codec.encode(msg);
        }

        static void destroy(AnyCodec& a) noexcept {
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

        static void move(AnyCodec& dst, AnyCodec& src) noexcept {
            dst.vtbl_ = src.vtbl_;

            if (src.heap_) {
                dst.obj_ = src.obj_;
                dst.heap_ = true;

                src.vtbl_ = nullptr;
                src.obj_ = nullptr;
                src.heap_ = false;
                return;
            }

            auto* src_m = static_cast<Model*>(src.obj_);
            auto* dst_m = ::new (static_cast<void*>(dst.storage_))
                Model(std::in_place, std::move(src_m->codec));

            dst.obj_ = dst_m;
            dst.heap_ = false;

            src_m->~Model();
            src.vtbl_ = nullptr;
            src.obj_ = nullptr;
            src.heap_ = false;
        }

        inline static const VTable vtbl{
            &Model::decode,
            &Model::encode,
            &Model::destroy,
            &Model::move,
        };
    };

    const VTable* vtbl_ = nullptr;
    void* obj_ = nullptr;
    bool heap_ = false;

    alignas(std::max_align_t) unsigned char storage_[kSboSize] = {};
};

} // namespace pcr::rpc
