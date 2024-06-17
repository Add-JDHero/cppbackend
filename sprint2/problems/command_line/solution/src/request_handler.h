#pragma once

#include "sdk.h"

#include "application.h"
#include "util.h"
#include "model.h"
#include "http_server.h"
#include "json_loader.h"
#include "url_parser.h"
#include "log.h"
#include "router.h"
#include "handlers.h"

#include <boost/json/serialize.hpp>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <filesystem>
#include <ctime>
#include <variant>
#include <boost/json.hpp>
#include <cassert>
    
using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace json = boost::json;
namespace fs = std::filesystem;


namespace http_handler {
    namespace separating_chars {
        constexpr const char SLASH = '/';
        constexpr const char BACK_SLASH = '\\';
    }

    bool IsSubPath(fs::path path, fs::path base);
    fs::path ProcessingAbsPath(std::string_view base, std::string_view rel);
    
    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;
    using FileResponse = http::response<http::file_body>;
    using EmptyResponse = http::response<http::empty_body>;

    using ResponseVariant = std::variant<EmptyResponse, StringResponse, FileResponse>;

    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
        constexpr static std::string_view APP_JSON = "application/json"sv;
    };

    struct SpecialStrings {
        SpecialStrings() = delete;

         constexpr static std::string_view AUTH_TOKEN = "authToken"sv;
         constexpr static std::string_view PLAYER_ID = "playerId"sv;
    };

    class ErrorHandler {
    public:

        /* static std::string ResponseBodyBuilder(std::string_view code, 
                                              std::string_view error_message); */

        static std::string SerializeErrorResponseBody(std::string_view code, 
                                                      std::string_view error_message);

        static StringResponse MakeBadRequestResponse(const JsonResponseHandler& json_response, 
                                                     std::string_view error_code = "",
                                                     std::string_view error_msg = "");
        static StringResponse MakeNotFoundResponse(const JsonResponseHandler& json_response, 
                                                   std::string_view error_code = "",
                                                   std::string_view error_msg = "");
        static StringResponse MakeNotAllowedResponse(const JsonResponseHandler& json_response,
                                                     std::vector<std::string> allowed_methods,
                                                     std::string_view error_code = "",
                                                     std::string_view error_msg = "");
        static StringResponse MakeUnauthorizedResponse(const JsonResponseHandler& json_response,
                                                     std::string_view error_code = "",
                                                     std::string_view error_msg = "");
    };

    class HttpResponse {
    public:
        static void MakeResponse(StringResponse& response, std::string body, bool keep_alive,
                                 std::string_view content_type = ContentType::TEXT_HTML);

        static StringResponse MakeStringResponse(http::status status, std::string body_sv,
                                                 unsigned http_version, bool keep_alive,
                                                 std::string_view content_type = 
                                                    ContentType::TEXT_HTML);
    };

    class ApiRequestHandler {
    public:
        ApiRequestHandler(app::Application& app);

        StringResponse RouteRequest(const StringRequest& req);

        StringResponse JoinGame(const StringRequest& req,
                                const JsonResponseHandler& json_response);

        StringResponse MoveUnit(const StringRequest& req, JsonResponseHandler json_response);

        StringResponse TickRequest(const StringRequest& req, 
                                   JsonResponseHandler json_response) const;

        StringResponse GetMapsRequest(const JsonResponseHandler& json_response) const;
        StringResponse GetMapDetailsRequest(const JsonResponseHandler& json_response,
                                            std::string_view map_id) const;
        StringResponse GetPlayersRequest(const StringRequest& req, 
                                         JsonResponseHandler json_response) const;
        StringResponse GetGameState(const StringRequest& req,
                                    JsonResponseHandler json_response) const;

    private:

        template <typename Fn>
        StringResponse ExecuteAuthorized(Fn&& action, const StringRequest& req,
                                         JsonResponseHandler json_response);

        bool IsValidAuthToken(std::string& auth_header) const;

        std::optional<StringResponse> 
        TokenHandler(const StringRequest& req, JsonResponseHandler json_response) const;
        
        std::optional<StringResponse> 
        TokenHandler(const StringRequest& req, 
                    JsonResponseHandler json_response,
                    std::string& token) const;

        std::optional<StringResponse> 
        IsAllowedMethod(const StringRequest& req, JsonResponseHandler json_response,
                        std::string message = {}) const;

        std::optional<StringResponse> 
        ParseJoinRequest(const StringRequest& req, JsonResponseHandler json_response,
                         json::object& obj) const;

        std::optional<StringResponse> 
        ParseContentType(const StringRequest& req, JsonResponseHandler json_response) const;

        std::optional<StringResponse> 
        ParseMoveJson(JsonResponseHandler json_response, std::string data,
                      std::string& direction) const;

        std::optional<StringResponse> 
        ParseTickJson(JsonResponseHandler json_response, std::string data,
                      uint64_t& milliseconds) const;

        void SetupEndPoits();

        fs::path root_dir_;
        app::Application& app_;

        std::unique_ptr<router::Router> router_;
    };

    class FileRequestHandler {
    public:
        FileRequestHandler(fs::path path);

        ResponseVariant HandleRequest(const StringRequest& request, 
                                      const JsonResponseHandler& json_response);

    private:
        fs::path root_dir_;
    };

    class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
    public:
        using Strand = net::strand<net::io_context::executor_type>;

        RequestHandler(Strand& api_strand, fs::path path, app::Application& app);

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        template <typename Send, typename Handler>
        void HandleRequest(StringRequest&& req, Send&& send, Handler json_response);

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);

    private:
        fs::path root_dir_;

        app::Application& app_;
        FileRequestHandler file_handler_{root_dir_};
        ApiRequestHandler api_handler_{app_};
        Strand& api_strand_;

        void SetupEndPoits();

        std::optional<StringResponse> 
        ParseJoinRequest(const StringRequest& req, JsonResponseHandler json_response,
                         json::object& obj) const;

        EmptyResponse CopyResponseWithoutBody(const ResponseVariant& response) const;
    };

    template<class SomeRequestHandler>
    class LoggingRequestHandler {

    public:
        LoggingRequestHandler(std::shared_ptr<SomeRequestHandler> handler) 
            : request_handler_(handler) {};

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);

    private:
        std::shared_ptr<SomeRequestHandler> request_handler_;

        template <typename Body, typename Allocator>
        static void LogRequest(http::request<Body, http::basic_fields<Allocator>>& req);
        static void LogResponse(EmptyResponse& response, int resp_duration);

        EmptyResponse CopyResponseWithoutBody(const ResponseVariant& response) const;
        
    };
}  // namespace http_handler

namespace http_handler {

    template <typename Body, typename Allocator, typename Send>
    void RequestHandler::operator()(http::request<Body, http::basic_fields<Allocator>>&& req, 
                                                Send&& send) {
        // ResponseVariant response = HandleRequest(std::move(req));
        auto ver = req.version();
        auto keep = req.keep_alive();
        auto json_response = 
            [this, version = ver, keep_alive = keep]
                (http::status status, std::string body = {},
                 std::string_view content_type = ContentType::APP_JSON) {
            return HttpResponse::MakeStringResponse(status, body, version, 
                                                    keep_alive, content_type);
        };

        HandleRequest(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send), 
                      json_response);
    }

    template <class SomeRequestHandler> 
    template <typename Body, typename Allocator, typename Send>
    void LoggingRequestHandler<SomeRequestHandler>::operator()(
                                http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        if (static_cast<std::string>(req.target()) != "/favicon.ico"){
            LogRequest(req);
            auto t1 = clock();
            request_handler_->operator()(std::move(req), [send = std::forward<Send>(send), this, t1]
                (ResponseVariant&& response) {
                auto empty_body_response = this->CopyResponseWithoutBody(response);
                std::visit([&send](auto&& result){
                    send(std::forward<decltype(result)>(result));
                }, response);
                auto t2 = clock();
                this->LogResponse(empty_body_response, int(t2 - t1));
            });

        }
    }

    template <class SomeRequestHandler> 
    template <typename Body, typename Allocator>
    void LoggingRequestHandler<SomeRequestHandler>::LogRequest(
            http::request<Body, http::basic_fields<Allocator>>& req) {
        boost::json::value jv = {
            {"URI", static_cast<std::string>(req.target())},
            {"method", req.method_string()}
        };
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, jv) << "request received";
    }

    template <class SomeRequestHandler> 
    void LoggingRequestHandler<SomeRequestHandler>::LogResponse(EmptyResponse& response, 
                                                                int resp_duration) {
        
        std::string content_type;
        auto content_type_iter = response.base().find(http::field::content_type);
        if (content_type_iter != response.base().end()) {
            content_type = content_type_iter->value();
        }

        json::value jv;
        jv = {
            {"response_time", resp_duration},
            {"code", response.result_int()},
            {"content_type", content_type}
        };

        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, jv) << "response sent";
    }

    template <class SomeRequestHandler>
    EmptyResponse LoggingRequestHandler<SomeRequestHandler>::
        CopyResponseWithoutBody(const ResponseVariant& response) const {
        EmptyResponse new_response;

        std::visit([&new_response](auto&& arg) {
            new_response.result(arg.result());
            new_response.version(arg.version());

            // Извлечение заголовка Content-Type
            auto content_type_iter = arg.base().find(http::field::content_type);
            if (content_type_iter != arg.base().end()) {
                new_response.set(http::field::content_type, content_type_iter->value());
            }

            // Копирование всех остальных заголовков
            for (auto& header : arg.base()) {
                new_response.set(header.name_string(), header.value());
            }
        }, response);

        return new_response;
    }

    template <typename Send, typename Handler>
    void RequestHandler::HandleRequest(StringRequest&& req, Send&& send, Handler json_response) {
        if (req.target().starts_with("/api")) {
            auto handle = [self = shared_from_this(), req = std::forward<decltype(req)>(req), send] {
                try {
                    // Этот assert не выстрелит, так как лямбда-функция будет выполняться 
                    // внутри strand
                    assert(self->api_strand_.running_in_this_thread());

                    ResponseVariant result = self->api_handler_.RouteRequest(req);
                    std::visit([&send](auto&& res){
                        res.set(http::field::cache_control, "no-cache");
                    }, result);

                    req.method_string() == "HEAD" 
                        ? result = self->CopyResponseWithoutBody(result) 
                        : result;

                    return send(std::forward<decltype(result)>(result));
                } catch (...) {
                    // ServerErrorLog(version, keep_alive));
                    
                }
            };

            return net::dispatch(api_strand_, handle);            
        }

        std::visit([&send](auto&& result){
            send(std::forward<decltype(result)>(result));
        }, file_handler_.HandleRequest(req, json_response));
    }

    template <typename Fn>
    StringResponse ApiRequestHandler::ExecuteAuthorized(Fn&& action, const StringRequest& req,
                                                        JsonResponseHandler json_response) {
        auto optional = TokenHandler(req, json_response);
        if (!optional.has_value()) {
            return action(req, json_response);
        } else {
            return optional.value();
        }
    }
}
