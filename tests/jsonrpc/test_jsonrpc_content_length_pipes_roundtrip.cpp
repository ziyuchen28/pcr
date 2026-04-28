#include "pcr/jsonrpc/dispatcher.h"

#include "pcr/framing/content_length_framer.h"
#include "pcr/framing/any_framer.h"

#include "pcr/stream/any_stream.h"
#include "pcr/stream/pipe_stream.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

#include <unistd.h>


static void make_pipe(int &r, int &w) 
{
    int fds[2] = {-1, -1};
#ifdef _WIN32
    // binary mode to avoid CRLF translation
    if (::_pipe(fds, 4096, _O_BINARY) != 0) {
        throw std::runtime_error("_pipe failed");
    }
#else
    if (::pipe(fds) != 0) {
        throw std::runtime_error("pipe() failed");
    }
#endif
    r = fds[0];
    w = fds[1];
}


int main() 
{
    using namespace pcr;


    int fd_a_to_b_r, fd_a_to_b_w;
    int fd_b_to_a_r, fd_b_to_a_w;

    make_pipe(fd_a_to_b_r, fd_a_to_b_w);
    make_pipe(fd_b_to_a_r, fd_b_to_a_w);

    stream::AnyStream A{stream::PipeDuplex(
        fd_b_to_a_r,
        fd_a_to_b_w,
        stream::FdOwnership::Owned,
        stream::FdOwnership::Owned
    )};

    stream::AnyStream B{stream::PipeDuplex(
        fd_a_to_b_r,
        fd_b_to_a_w,
        stream::FdOwnership::Owned,
        stream::FdOwnership::Owned
    )};

    jsonrpc::Dispatcher cdisp(framing::AnyFramer{framing::ContentLengthFramer(A)});
    jsonrpc::Dispatcher sdisp(framing::AnyFramer{framing::ContentLengthFramer(B)});

    // Echo params back as result
    sdisp.on_request("echo", [](const jsonrpc::Request &req) {
        return jsonrpc::HandlerResult::ok(req.params_json.value_or("null"));
    });

    const std::string params = R"({"msg":"hi","n":7})";

    const jsonrpc::Id id = cdisp.send_request("echo", params);

    // Server handles request -> sends response
    assert(sdisp.pump_once());

    // Client reads response
    assert(cdisp.pump_once());

    auto resp = cdisp.take_response(id);
    assert(resp.has_value());
    assert(!resp->error.has_value());
    assert(resp->result_json.has_value());
    assert(*resp->result_json == params);

    std::cout << "test_rpc_content_length_pipes_roundtrip: ok\n";
    return 0;
}



