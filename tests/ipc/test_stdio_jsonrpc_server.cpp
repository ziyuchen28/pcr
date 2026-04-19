#include "pcr/jsonrpc/dispatcher.h"
#include "pcr/framing/any_framer.h"
#include "pcr/framing/content_length_framer.h"
#include "pcr/stream/pipe_stream.h"
#include "pcr/stream/any_stream.h"

#include <string>
#include <unistd.h>

int main()
{
    using namespace pcr;

    // read from stdin, write to stdout.
    stream::AnyStream io(stream::PipeDuplex(
        STDIN_FILENO,
        STDOUT_FILENO,
        stream::FdOwnership::Borrowed,
        stream::FdOwnership::Borrowed));

    jsonrpc::Dispatcher rpc(
        framing::AnyFramer{
            framing::ContentLengthFramer(io)
        });

    rpc.on_request("echo", [](const jsonrpc::Request &req) {
        return jsonrpc::HandlerResult::ok(
            req.params_json.value_or("null"));
    });

    rpc.on_request("trigger_notify", [&](const jsonrpc::Request&) {
        rpc.send_notification(
            "server/hello",
            R"({"msg":"hi from server"})");
        return jsonrpc::HandlerResult::ok("null");
    });

    // Pump until client closes stdin -> EOF.
    while (rpc.pump_once()) {
    }

    return 0;
}
