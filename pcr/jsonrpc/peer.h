#pragma once

#include "pcr/framing/any_framer.h"
#include "pcr/jsonrpc/any_codec.h"
#include "pcr/jsonrpc/metrics.h"
#include "pcr/jsonrpc/message.h"

#include <optional>

namespace pcr::jsonrpc {

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

} // namespace pcr::jsonrpc
