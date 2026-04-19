#include "pcr/ipc/stdio_jsonrpc_session.h"

#include <stdexcept>
#include <utility>

#include "pcr/proc/piped_child.h"
#include "pcr/proc/proc_spec.h"
#include "pcr/framing/any_framer.h"
#include "pcr/framing/content_length_framer.h"
#include "pcr/proc/piped_child.h"
#include "pcr/proc/proc_spec.h"
#include "pcr/stream/pipe_stream.h"
#include "pcr/jsonrpc/id.h"


namespace pcr::ipc {

namespace {

pcr::proc::ProcessSpec to_process_spec(const StdioJsonRpcLaunchConfig &config)
{
    pcr::proc::ProcessSpec spec;
    spec.exe = config.exe;
    spec.args = config.args;
    if (config.cwd.has_value()) {
        spec.cwd = *config.cwd;
    }
    return spec;
}

} // namespace


struct StdioJsonRpcSession::Impl
{
    pcr::proc::PipedChild child;
    pcr::stream::AnyStream io;
    pcr::jsonrpc::Dispatcher rpc;
    bool closed{false};

    explicit Impl(pcr::proc::PipedChild c)
        : child(std::move(c)),
          io(pcr::stream::PipeDuplex(
              child.stdout_read_fd(),
              child.stdin_write_fd(),
              pcr::stream::FdOwnership::Borrowed,
              pcr::stream::FdOwnership::Borrowed)),
          rpc(pcr::framing::AnyFramer{
              pcr::framing::ContentLengthFramer(io)})
    {
    }
};

StdioJsonRpcSession::StdioJsonRpcSession(std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl))
{
}
StdioJsonRpcSession::StdioJsonRpcSession(StdioJsonRpcSession&&) noexcept = default;
StdioJsonRpcSession &StdioJsonRpcSession::operator=(StdioJsonRpcSession&&) noexcept = default;
StdioJsonRpcSession::~StdioJsonRpcSession() = default;



std::string StdioJsonRpcSession::request(
    std::string method, 
    std::optional<std::string> params_json)
{
    if (impl_->closed) {
        throw std::logic_error("stdio jsonrpc session is closed");
    }

    const std::string method_for_error = method;
    const pcr::jsonrpc::Id id =
        impl_->rpc.send_request(std::move(method), std::move(params_json));

    for (;;) {
        if (auto response = impl_->rpc.take_response(id); response.has_value()) {
            if (response->error.has_value()) {
                throw std::runtime_error(
                    "json-rpc request '" + method_for_error + "' failed: " +
                    response->error->message);
            }
            return response->result_json.value_or("null");
        }

        if (!impl_->rpc.pump_once()) {
            throw std::runtime_error(
                "json-rpc eof while waiting for response to '" +
                method_for_error + "'");
        }
    }
}


void StdioJsonRpcSession::notify(
    std::string method, 
    std::optional<std::string> params_json)
{
    if (impl_->closed) {
        throw std::logic_error("stdio jsonrpc session is closed");
    }
    impl_->rpc.send_notification(std::move(method), std::move(params_json));
}


void StdioJsonRpcSession::on_request(
    std::string method, 
    RequestHandler handler)
{
    impl_->rpc.on_request(std::move(method), std::move(handler));
}


void StdioJsonRpcSession::on_notification(
    std::string method,
    NotificationHandler handler)
{
    impl_->rpc.on_notification(std::move(method), std::move(handler));
}


void StdioJsonRpcSession::close()
{
    if (impl_->closed) {
        return;
    }
    impl_->child.close_stdin_write();
    impl_->closed = true;
}


void StdioJsonRpcSession::wait()
{
    (void)impl_->child.wait();
}


StdioJsonRpcSession StdioJsonRpcSession::spawn(const StdioJsonRpcLaunchConfig &config)
{
    return StdioJsonRpcSession(
        std::make_unique<StdioJsonRpcSession::Impl>(
            pcr::proc::PipedChild::spawn(to_process_spec(config))
        )
    );
}

} // namespace pcr::ipc

