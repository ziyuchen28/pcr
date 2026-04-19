#pragma once

#include <memory>
#include <optional>
#include <string>

#include "pcr/jsonrpc/dispatcher.h"

namespace pcr::ipc {

struct StdioJsonRpcLaunchConfig
{
    std::string exe;
    std::vector<std::string> args;
    std::optional<std::string> cwd;
};

class StdioJsonRpcSession
{
public:
    static StdioJsonRpcSession spawn(const StdioJsonRpcLaunchConfig &cfg);

    StdioJsonRpcSession(StdioJsonRpcSession&&) noexcept;
    StdioJsonRpcSession &operator=(StdioJsonRpcSession&&) noexcept;
    ~StdioJsonRpcSession();

    StdioJsonRpcSession(const StdioJsonRpcSession&) = delete;
    StdioJsonRpcSession &operator=(const StdioJsonRpcSession&) = delete;

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

    // session lifecycle
    void close();
    void wait();

private:
    struct Impl;
    explicit StdioJsonRpcSession(std::unique_ptr<Impl> impl) noexcept;
    std::unique_ptr<Impl> impl_;
};

// StdioJsonRpcSession spawn_stdio_jsonrpc(const StdioJsonRpcLaunchConfig &config);

} // namespace pcr::ipc
