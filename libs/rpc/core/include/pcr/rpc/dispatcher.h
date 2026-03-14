#pragma once

#include "pcr/rpc/error.h"
#include "pcr/rpc/id.h"
#include "pcr/rpc/message.h"
#include "pcr/rpc/metrics.h"
#include "pcr/rpc/peer.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>


namespace pcr::rpc {


struct HandlerResult 
{
    std::optional<std::string> result_json; // raw JSON value
    std::optional<Error> error;

    static HandlerResult ok(std::string result_json_) 
    {
        HandlerResult hr;
        hr.result_json = std::move(result_json_);
        return hr;
    }

    static HandlerResult fail(Error e) 
    {
        HandlerResult hr;
        hr.error = std::move(e);
        return hr;
    }
};


class Dispatcher 
{
public:
    using RequestHandler = std::function<HandlerResult(const Request&)>;
    using NotificationHandler = std::function<void(const Notification&)>;

    explicit Dispatcher(Peer peer, MetricsSink *metrics = nullptr);

    // client side
    Id send_request(std::string method, std::optional<std::string> params_json = std::nullopt);
    void send_notification(std::string method, std::optional<std::string> params_json = std::nullopt);

    // server side
    void on_request(std::string method, RequestHandler h);
    void on_notification(std::string method, NotificationHandler h);

    // pump exactly one message (one frame).
    // returns false on clean EOF.
    bool pump_once();

    // retrieve a response already received
    std::optional<Response> take_response(const Id &id);

private:
    void handle_request(const Request &r);
    void handle_notification(const Notification &n);
    void handle_response(Response &&r);

    void send_result(const Id &id, std::string result_json);
    void send_error(const Id &id, Error e);

    Peer peer_;
    MetricsSink *metrics_;

    std::int64_t next_id_ = 1;

    std::unordered_map<std::string, RequestHandler> request_handlers_;
    std::unordered_map<std::string, NotificationHandler> notification_handlers_;
    std::unordered_map<Id, Response, IdHash> responses_;
};

} // namespace pcr::rpc



