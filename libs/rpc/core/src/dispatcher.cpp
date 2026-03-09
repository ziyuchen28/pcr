#include "pcr/rpc/dispatcher.h"

#include "pcr/rpc/clock.h"

#include <stdexcept>
#include <utility>

namespace pcr::rpc {

Dispatcher::Dispatcher(Peer peer, MetricsSink* metrics)
    : peer_(std::move(peer)),
      metrics_(metrics) {}

Id Dispatcher::send_request(std::string method, std::optional<std::string> params_json) {
    const Id id = Id::from_int(next_id_++);

    Request r;
    r.id = id;
    r.method = std::move(method);
    r.params_json = std::move(params_json);

    peer_.write(Message{r});
    metric_counter(metrics_, Metric::RequestsSent, 1);
    return id;
}

void Dispatcher::send_notification(std::string method, std::optional<std::string> params_json) {
    Notification n;
    n.method = std::move(method);
    n.params_json = std::move(params_json);

    peer_.write(Message{n});
    metric_counter(metrics_, Metric::NotificationsSent, 1);
}

void Dispatcher::on_request(std::string method, RequestHandler h) {
    request_handlers_[std::move(method)] = std::move(h);
}

void Dispatcher::on_notification(std::string method, NotificationHandler h) {
    notification_handlers_[std::move(method)] = std::move(h);
}

bool Dispatcher::pump_once() {
    const std::uint64_t t0 = now_ns();

    auto msg = peer_.read();
    if (!msg.has_value()) {
        return false;
    }

    std::visit([&](auto&& m) {
        using T = std::decay_t<decltype(m)>;
        if constexpr (std::is_same_v<T, Request>) {
            metric_counter(metrics_, Metric::RequestsRecv, 1);
            handle_request(m);
        } else if constexpr (std::is_same_v<T, Notification>) {
            metric_counter(metrics_, Metric::NotificationsRecv, 1);
            handle_notification(m);
        } else if constexpr (std::is_same_v<T, Response>) {
            metric_counter(metrics_, Metric::ResponsesRecv, 1);
            handle_response(std::move(m));
        }
    }, *msg);

    const std::uint64_t t1 = now_ns();
    metric_timing(metrics_, Metric::PumpOnceNs, t1 - t0);
    return true;
}

std::optional<Response> Dispatcher::take_response(const Id& id) {
    auto it = responses_.find(id);
    if (it == responses_.end()) return std::nullopt;
    Response out = std::move(it->second);
    responses_.erase(it);
    return out;
}

void Dispatcher::handle_request(const Request& r) {
    auto it = request_handlers_.find(r.method);
    if (it == request_handlers_.end()) {
        Error e;
        e.code = kMethodNotFound;
        e.message = "Method not found: " + r.method;
        send_error(r.id, std::move(e));
        return;
    }

    const std::uint64_t t0 = now_ns();
    HandlerResult hr;
    try {
        hr = it->second(r);
    } catch (const std::exception& ex) {
        Error e;
        e.code = kInternalError;
        e.message = std::string("Handler exception: ") + ex.what();
        hr = HandlerResult::fail(std::move(e));
    }
    const std::uint64_t t1 = now_ns();
    metric_timing(metrics_, Metric::HandlerNs, t1 - t0);

    const bool has_res = hr.result_json.has_value();
    const bool has_err = hr.error.has_value();
    if (has_res == has_err) {
        Error e;
        e.code = kInternalError;
        e.message = "HandlerResult invalid (must set exactly one of result_json or error)";
        send_error(r.id, std::move(e));
        return;
    }

    if (has_err) send_error(r.id, std::move(*hr.error));
    else         send_result(r.id, std::move(*hr.result_json));
}

void Dispatcher::handle_notification(const Notification& n) {
    auto it = notification_handlers_.find(n.method);
    if (it == notification_handlers_.end()) return;

    try { it->second(n); }
    catch (...) { /* notifications are fire-and-forget */ }
}

void Dispatcher::handle_response(Response&& r) {
    responses_[r.id] = std::move(r);
}

void Dispatcher::send_result(const Id& id, std::string result_json) {
    Response r;
    r.id = id;
    r.result_json = std::move(result_json);
    peer_.write(Message{r});
    metric_counter(metrics_, Metric::ResponsesSent, 1);
}

void Dispatcher::send_error(const Id& id, Error e) {
    Response r;
    r.id = id;
    r.error = std::move(e);
    peer_.write(Message{r});
    metric_counter(metrics_, Metric::ResponsesSent, 1);
}

} // namespace pcr::rpc
