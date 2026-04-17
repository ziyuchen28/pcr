#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <pcr/proc/piped_child.h>
#include <pcr/proc/proc_spec.h>
#include <pcr/jsonrpc/dispatcher.h>
#include <pcr/jsonrpc/id.h>
#include <pcr/jsonrpc/message.h>

namespace pcr::ipc {

using RequestHandler =
    std::function<pcr::jsonrpc::HandlerResult(const pcr::jsonrpc::Request&)>;

using NotificationHandler =
    std::function<void(const pcr::jsonrpc::Notification&)>;

// Recommended path:
//   auto s = pcr::ipc::spawn_stdio_jsonrpc(spec);
//
// Expert path:
//   auto s = pcr::ipc::StdioJsonRpcSession::attach(std::move(child));
class StdioJsonRpcSession {
public:
    static StdioJsonRpcSession attach(pcr::proc::PipedChild child);

    StdioJsonRpcSession(StdioJsonRpcSession&&) noexcept;
    StdioJsonRpcSession& operator=(StdioJsonRpcSession&&) noexcept;
    ~StdioJsonRpcSession();

    StdioJsonRpcSession(const StdioJsonRpcSession&) = delete;
    StdioJsonRpcSession& operator=(const StdioJsonRpcSession&) = delete;

    pcr::jsonrpc::Id send_request(std::string method,
                              std::optional<std::string> params_json = std::nullopt);

    void send_notification(std::string method,
                           std::optional<std::string> params_json = std::nullopt);

    void on_request(std::string method, RequestHandler handler);
    void on_notification(std::string method, NotificationHandler handler);

    bool pump_once();
    std::optional<pcr::jsonrpc::Response> take_response(const pcr::jsonrpc::Id& id);

    pcr::jsonrpc::Response request(std::string method,
                               std::optional<std::string> params_json = std::nullopt);

    std::string request_json(std::string method,
                             std::optional<std::string> params_json = std::nullopt,
                             const char* error_prefix = "JSON-RPC request failed");

    void close_stdin();
    int stderr_read_fd() const noexcept;
    pcr::proc::WaitResult wait();

private:
    struct Impl;
    explicit StdioJsonRpcSession(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> impl_;
};

StdioJsonRpcSession spawn_stdio_jsonrpc(pcr::proc::ProcessSpec spec);

} // namespace pcr::ipc
