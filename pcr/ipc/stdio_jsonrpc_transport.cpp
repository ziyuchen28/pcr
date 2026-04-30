#include "pcr/ipc/stdio_jsonrpc_transport.h"

#include <stdexcept>
#include <utility>
#include <csignal>

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


struct StdioJsonRpcTransport::Impl
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

StdioJsonRpcTransport::StdioJsonRpcTransport(std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}
StdioJsonRpcTransport::StdioJsonRpcTransport(StdioJsonRpcTransport&&) noexcept = default;
StdioJsonRpcTransport &StdioJsonRpcTransport::operator=(StdioJsonRpcTransport&&) noexcept = default;
StdioJsonRpcTransport::~StdioJsonRpcTransport() = default;


std::string StdioJsonRpcTransport::request(
    std::string method, 
    std::optional<std::string> params_json)
{
    if (impl_->closed) {
        throw std::logic_error("stdio jsonrpc transport is closed");
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


void StdioJsonRpcTransport::notify(
    std::string method, 
    std::optional<std::string> params_json)
{
    if (impl_->closed) {
        throw std::logic_error("stdio jsonrpc transport is closed");
    }
    impl_->rpc.send_notification(std::move(method), std::move(params_json));
}


void StdioJsonRpcTransport::on_request(
    std::string method, 
    pcr::jsonrpc::Dispatcher::RequestHandler handler)
{
    impl_->rpc.on_request(std::move(method), std::move(handler));
}


void StdioJsonRpcTransport::on_notification(
    std::string method,
    pcr::jsonrpc::Dispatcher::NotificationHandler handler)
{
    impl_->rpc.on_notification(std::move(method), std::move(handler));
}


void StdioJsonRpcTransport::close()
{
    if (impl_->closed) {
        return;
    }
    impl_->child.close_stdin_write();
    impl_->closed = true;
}


void StdioJsonRpcTransport::wait()
{
    (void)impl_->child.wait();
}

bool StdioJsonRpcTransport::wait_for(std::chrono::milliseconds timeout)
{
    return impl_->child.wait_for(timeout).has_value();
}

void StdioJsonRpcTransport::terminate()
{
    impl_->child.terminate(SIGTERM);
}

void StdioJsonRpcTransport::kill()
{
#ifdef _WIN32
    impl_->child.terminate(SIGTERM);
#else
    impl_->child.terminate(SIGKILL);
#endif
}
StdioJsonRpcTransport StdioJsonRpcTransport::spawn(const StdioJsonRpcLaunchConfig &config)
{
    return StdioJsonRpcTransport(
        std::make_unique<StdioJsonRpcTransport::Impl>(
            pcr::proc::PipedChild::spawn_inherit_stderr(to_process_spec(config))
        )
    );
}

} // namespace pcr::ipc

