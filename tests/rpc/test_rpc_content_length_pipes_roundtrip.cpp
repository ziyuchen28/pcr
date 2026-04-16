#include "dispatcher.h"
#include "peer.h"
#include "nlohmann.h"

#include "content_length_framer.h"
#include "any_framer.h"

#include "any_stream.h"
#include "pipe_stream.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

#include <unistd.h>

namespace {

struct UniqueFd 
{
    int fd = -1;
    explicit UniqueFd(int f = -1) : fd(f) {}
    ~UniqueFd() { if (fd >= 0) ::close(fd); }
    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;
    UniqueFd(UniqueFd&& o) noexcept : fd(o.fd) { o.fd = -1; }
    UniqueFd& operator=(UniqueFd&& o) noexcept {
        if (this == &o) return *this;
        if (fd >= 0) ::close(fd);
        fd = o.fd; o.fd = -1;
        return *this;
    }
    int release() noexcept { int t = fd; fd = -1; return t; }
};

void make_pipe(UniqueFd &r, UniqueFd &w) 
{
    int fds[2] = {-1, -1};
    if (::pipe(fds) != 0) throw std::runtime_error("pipe() failed");
    r = UniqueFd(fds[0]);
    w = UniqueFd(fds[1]);
}

} // namespace



int main() 
{
    using namespace pcr;

    // Full duplex = two pipes (like parent<->child stdio)
    UniqueFd a_to_b_r, a_to_b_w;
    UniqueFd b_to_a_r, b_to_a_w;
    make_pipe(a_to_b_r, a_to_b_w);
    make_pipe(b_to_a_r, b_to_a_w);

    channel::AnyStream A{channel::PipeDuplex(
        b_to_a_r.release(),
        a_to_b_w.release(),
        channel::FdOwnership::Owned,
        channel::FdOwnership::Owned
    )};

    channel::AnyStream B{channel::PipeDuplex(
        a_to_b_r.release(),
        b_to_a_w.release(),
        channel::FdOwnership::Owned,
        channel::FdOwnership::Owned
    )};

    // Build peers
    rpc::Peer client(
        framing::AnyFramer{framing::ContentLengthFramer(A)},
        rpc::AnyCodec{rpc::NlohmannCodec{}}
    );

    rpc::Peer server(
        framing::AnyFramer{framing::ContentLengthFramer(B)},
        rpc::AnyCodec{rpc::NlohmannCodec{}}
    );

    rpc::Dispatcher cdisp(std::move(client));
    rpc::Dispatcher sdisp(std::move(server));

    // Echo params back as result
    sdisp.on_request("echo", [](const rpc::Request &req) {
        return rpc::HandlerResult::ok(req.params_json.value_or("null"));
    });

    // Create params JSON string using nlohmann helper
    const std::string params = rpc::json_dump(nlohmann::json{{"msg","hi"},{"n",7}});

    const rpc::Id id = cdisp.send_request("echo", params);

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



