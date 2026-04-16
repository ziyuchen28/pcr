#pragma once

#include "any_framer.h"
#include "any_codec.h"
#include "metrics.h"
#include "message.h"

#include <optional>

namespace pcr::rpc {

class Peer 
{
public:
    Peer(pcr::framing::AnyFramer framer,
         AnyCodec codec,
         MetricsSink *metrics = nullptr);

    // nullopt => clean EOF
    std::optional<Message> read();

    void write(const Message &msg);

private:
    pcr::framing::AnyFramer framer_;
    AnyCodec codec_;
    MetricsSink *metrics_;
};

} // namespace pcr::rpc
