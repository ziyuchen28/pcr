#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace pcr::jsonrpc {

struct Id 
{
    enum class Kind : std::uint8_t 
    { 
        Null, Int, String 
    };

    Kind kind = Kind::Null;
    // jsonrpc allows neg integer 
    std::int64_t i = 0;
    std::string s;

    // couple named constructors
    static Id null() noexcept 
    { 
        return {}; 
    }

    static Id from_int(std::int64_t v) noexcept 
    {
        Id id;
        id.kind = Kind::Int;
        id.i = v;
        return id;
    }

    static Id from_string(std::string v) 
    {
        Id id;
        id.kind = Kind::String;
        id.s = std::move(v);
        return id;
    }
};

// used together with IdHash
inline bool operator==(const Id &a, const Id &b) noexcept 
{
    if (a.kind != b.kind) return false;
    switch (a.kind) {
        case Id::Kind::Null:   return true;
        case Id::Kind::Int:    return a.i == b.i;
        case Id::Kind::String: return a.s == b.s;
    }
    return false;
}

// used in map
struct IdHash 
{
    std::size_t operator()(const Id &id) const noexcept 
    {
        // wtf? FNV-1a hash offset basis
        std::size_t h = 1469598103934665603ull;
        auto mix = [&](std::size_t x) {
            // golden Ratio fraction for 64-bit integers
            h ^= x + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
        };
        // in case nulll and from_int(0) conflict
        mix(static_cast<std::size_t>(id.kind));
        switch (id.kind) {
            case Id::Kind::Null:   mix(0); break;
            case Id::Kind::Int:    mix(std::hash<std::int64_t>{}(id.i)); break;
            case Id::Kind::String: mix(std::hash<std::string>{}(id.s)); break;
        }
        return h;
    }
};

} // namespace pcr::jsonrpc
