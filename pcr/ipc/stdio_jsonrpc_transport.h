#pragma once

#include <memory>
#include <optional>
#include <string>
#include <chrono>

#include "pcr/jsonrpc/dispatcher.h"

namespace pcr::ipc {

struct StdioJsonRpcLaunchConfig
{
    std::string exe;
    std::vector<std::string> args;
    std::optional<std::string> cwd;
};

class StdioJsonRpcTransport
{
public:
    static StdioJsonRpcTransport spawn(const StdioJsonRpcLaunchConfig &cfg);

    StdioJsonRpcTransport(StdioJsonRpcTransport&&) noexcept;
    StdioJsonRpcTransport &operator=(StdioJsonRpcTransport&&) noexcept;
    ~StdioJsonRpcTransport();

    StdioJsonRpcTransport(const StdioJsonRpcTransport&) = delete;
    StdioJsonRpcTransport &operator=(const StdioJsonRpcTransport&) = delete;

    // blocking request/response 
    // throws on JSON-RPC error response or EOF
    std::string request(
        std::string method,
        std::optional<std::string> params_json = std::nullopt);

    // fire-and-forget notification
    void notify(
        std::string method,
        std::optional<std::string> params_json = std::nullopt);

    // incoming server->client traffic handlers
    void on_request(std::string method, pcr::jsonrpc::Dispatcher::RequestHandler handler);
    void on_notification(std::string method, pcr::jsonrpc::Dispatcher::NotificationHandler handler);

    // Transport lifecycle
    void close();
    void wait();
    bool wait_for(std::chrono::milliseconds timeout);
    void terminate();
    void kill();

private:
    struct Impl;
    explicit StdioJsonRpcTransport(std::unique_ptr<Impl> impl) noexcept;
    std::unique_ptr<Impl> impl_;
};

// StdioJsonRpcTransport spawn_stdio_jsonrpc(const StdioJsonRpcLaunchConfig &config);

} // namespace pcr::ipc
