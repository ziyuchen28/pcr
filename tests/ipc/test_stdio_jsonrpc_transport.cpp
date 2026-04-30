#include "pcr/ipc/stdio_jsonrpc_transport.h"

#include <cassert>
#include <iostream>
#include <optional>
#include <string>

#ifndef TEST_STDIO_JSONRPC_SERVER
#error "TEST_STDIO_JSONRPC_SERVER not defined"
#endif

int main()
{
    pcr::ipc::StdioJsonRpcLaunchConfig cfg;
    cfg.exe = TEST_STDIO_JSONRPC_SERVER;

    auto transport = pcr::ipc::StdioJsonRpcTransport::spawn(cfg);

    // basic request/response.
    const std::string params = R"({"msg":"hi","n":7})";
    const std::string result = transport.request("echo", params);
    assert(result == params);

    // notification callback while request is pumping.
    bool got_notify = false;
    transport.on_notification("server/hello",
        [&](const pcr::jsonrpc::Notification &n) {
            got_notify = true;
            assert(n.params_json.has_value());
            assert(*n.params_json == R"({"msg":"hi from server"})");
        });

    const std::string result2 =
        transport.request("trigger_notify", std::nullopt);

    assert(result2 == "null");
    assert(got_notify);

    transport.close();
    transport.wait();

    std::cout << "test_stdio_jsonrpc_transport: ok\n";
    return 0;
}
