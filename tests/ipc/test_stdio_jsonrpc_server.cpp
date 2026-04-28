#include "pcr/jsonrpc/dispatcher.h"
#include "pcr/framing/any_framer.h"
#include "pcr/framing/content_length_framer.h"
#include "pcr/stream/pipe_stream.h"
#include "pcr/stream/any_stream.h"

#include <string>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#endif

int main()
{
    using namespace pcr;

#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
    int in_fd = _fileno(stdin);
    int out_fd = _fileno(stdout);
#else
    int in_fd = STDIN_FILENO;
    int out_fd = STDOUT_FILENO;
#endif

    // read from stdin, write to stdout.
    stream::AnyStream io(stream::PipeDuplex(
        in_fd,
        out_fd,
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
