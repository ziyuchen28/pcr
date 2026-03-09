#pragma once

#include "pcr/framing/framer.h"

#include <cstddef>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace pcr::framing {

// Move-only type-erased Framer with SBO.
// - Inline store if T fits and is nothrow-movable
// - Else heap store (move is pointer move, noexcept)
class AnyFramer {
public:
    AnyFramer() noexcept = default;

    AnyFramer(const AnyFramer&) = delete;
    AnyFramer& operator=(const AnyFramer&) = delete;

    AnyFramer(AnyFramer&& other) noexcept {
        if (other.vtbl_) other.vtbl_->move(*this, other);
    }

    AnyFramer& operator=(AnyFramer&& other) noexcept {
        if (this == &other) return *this;
        reset();
        if (other.vtbl_) other.vtbl_->move(*this, other);
        return *this;
    }

    ~AnyFramer() noexcept { reset(); }

    template <class F>
        requires Framer<F>
    AnyFramer(F f) {
        emplace<F>(std::move(f));
    }

    template <class F, class... Args>
        requires Framer<F>
    F& emplace(Args&&... args) {
        reset();
        using M = Model<F>;

        constexpr bool can_inline =
            fits_sbo<M>() && std::is_nothrow_move_constructible_v<F>;

        if constexpr (can_inline) {
            obj_ = ::new (static_cast<void*>(storage_)) M(std::in_place, std::forward<Args>(args)...);
            heap_ = false;
        } else {
            obj_ = new M(std::in_place, std::forward<Args>(args)...);
            heap_ = true;
        }

        vtbl_ = &M::vtbl;
        return static_cast<M*>(obj_)->framer;
    }

    void reset() noexcept {
        if (vtbl_) vtbl_->destroy(*this);
    }

    explicit operator bool() const noexcept { return vtbl_ != nullptr; }

    std::optional<std::string> read_frame() {
        if (!vtbl_) throw std::logic_error("AnyFramer: read_frame on empty");
        return vtbl_->read_frame(obj_);
    }

    void write_frame(std::string_view sv) {
        if (!vtbl_) throw std::logic_error("AnyFramer: write_frame on empty");
        vtbl_->write_frame(obj_, sv);
    }

private:
    static constexpr std::size_t kSboSize = 128;

    struct VTable {
        std::optional<std::string> (*read_frame)(void*);
        void (*write_frame)(void*, std::string_view);
        void (*destroy)(AnyFramer&) noexcept;
        void (*move)(AnyFramer& dst, AnyFramer& src) noexcept;
    };

    template <class M>
    static consteval bool fits_sbo() {
        return sizeof(M) <= kSboSize && alignof(M) <= alignof(std::max_align_t);
    }

    template <class F>
    struct Model {
        F framer;

        template <class... Args>
        explicit Model(std::in_place_t, Args&&... args)
            : framer(std::forward<Args>(args)...) {}

        static std::optional<std::string> read_frame(void* self) {
            return static_cast<Model*>(self)->framer.read_frame();
        }

        static void write_frame(void* self, std::string_view sv) {
            static_cast<Model*>(self)->framer.write_frame(sv);
        }

        static void destroy(AnyFramer& a) noexcept {
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

        static void move(AnyFramer& dst, AnyFramer& src) noexcept {
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
                Model(std::in_place, std::move(src_m->framer));

            dst.obj_ = dst_m;
            dst.heap_ = false;

            src_m->~Model();
            src.vtbl_ = nullptr;
            src.obj_ = nullptr;
            src.heap_ = false;
        }

        inline static const VTable vtbl{
            &Model::read_frame,
            &Model::write_frame,
            &Model::destroy,
            &Model::move,
        };
    };

    const VTable* vtbl_ = nullptr;
    void* obj_ = nullptr;
    bool heap_ = false;

    alignas(std::max_align_t) unsigned char storage_[kSboSize] = {};
};

} // namespace pcr::framing
