#pragma once

#include "pcr/framing/any_framer.h"
#include "pcr/rpc/any_codec.h"
#include "pcr/rpc/metrics.h"
#include "pcr/rpc/message.h"

#include <optional>

namespace pcr::rpc {

class Peer {
public:
    Peer(pcr::framing::AnyFramer framer,
         AnyCodec codec,
         MetricsSink* metrics = nullptr);

    // nullopt => clean EOF
    std::optional<Message> read();

    void write(const Message& msg);

private:
    pcr::framing::AnyFramer framer_;
    AnyCodec codec_;
    MetricsSink* metrics_;
};

} // namespace pcr::rpc
