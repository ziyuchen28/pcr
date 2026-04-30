#include "pcr/ipc/stdio_jsonrpc_transport.h"

#include <cassert>
#include <csignal>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>

#ifndef PCR_TEST_STDERR_FLOOD_SERVER
#error "PCR_TEST_STDERR_FLOOD_SERVER must be defined"
#endif

namespace {

void redirect_parent_stderr_to_null()
{
    const int fd = ::open("/dev/null", O_WRONLY);
    if (fd < 0) {
        throw std::runtime_error("failed to open /dev/null");
    }

    if (::dup2(fd, STDERR_FILENO) < 0) {
        ::close(fd);
        throw std::runtime_error("failed to redirect stderr to /dev/null");
    }

    ::close(fd);
}

} // namespace


int main()
{

    // child will inherite stderr goes to /dev/null.
    redirect_parent_stderr_to_null();

    pcr::ipc::StdioJsonRpcLaunchConfig cfg;
    cfg.exe = PCR_TEST_STDERR_FLOOD_SERVER;

    auto transport = pcr::ipc::StdioJsonRpcTransport::spawn(cfg);

    const std::string result = transport.request("ping");
    assert(result == R"({"pong":true})");

    transport.close();
    transport.wait();

    std::cout << "test_stdio_jsonrpc_inherit_stderr: ok\n";
    return 0;
}
