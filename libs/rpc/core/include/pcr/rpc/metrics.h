#pragma once

#include <cstdint>

namespace pcr::rpc {

// Keep the hook API “C-like” (systems-y): no virtuals, no allocations.
enum class Metric : std::uint8_t {
    FramesIn,
    FramesOut,
    BytesIn,
    BytesOut,

    DecodeNs,
    EncodeNs,

    PumpOnceNs,
    HandlerNs,

    RequestsSent,
    NotificationsSent,
    ResponsesSent,

    RequestsRecv,
    NotificationsRecv,
    ResponsesRecv,
};

struct MetricsSink {
    void* ctx = nullptr;

    // Counters: add delta.
    void (*counter)(void* ctx, Metric m, std::uint64_t delta) = nullptr;

    // Timings: record duration (ns).
    void (*timing)(void* ctx, Metric m, std::uint64_t ns) = nullptr;
};

inline void metric_counter(MetricsSink* s, Metric m, std::uint64_t d) noexcept {
    if (s && s->counter) s->counter(s->ctx, m, d);
}

inline void metric_timing(MetricsSink* s, Metric m, std::uint64_t ns) noexcept {
    if (s && s->timing) s->timing(s->ctx, m, ns);
}

} // namespace pcr::rpc
