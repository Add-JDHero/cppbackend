#pragma once

#include "sdk.h" 
#include "http_server.h"
#include "json_loader.h"
#include "model.h"
#include "url_parser.h"
#include "util.h"
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

    bool IsAllowedReqMethod(beast::http::verb method);

    fs::path ProcessingAbsPath(std::string_view base, std::string_view rel);
    
    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;

    using FileResponse = http::response<http::file_body>;
    using EmptyResponse = http::response<http::empty_body>;
    using ResponseVariant = std::variant<StringResponse, FileResponse>;

    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
        constexpr static std::string_view APP_JSON = "application/json"sv;
        // При необходимости внутрь ContentType можно добавить и другие типы контента
    };

    class ErrorHandler {
    public:
        static std::string BadRequest(std::string_view code = "badRequest", 
                                    std::string_view error_message = "Bad Request");
        static std::string NotFound(std::string_view code = "notFound",
                                    std::string_view error_message = "Not Found");
        
        template <typename Handler>
        static StringResponse MakeBadRequestResponse(const Handler& json_response, 
                                                     std::string_view error_code = "",
                                                     std::string_view error_msg = "") {
            std::string response_body = 
                error_code.empty() ? BadRequest() : BadRequest(error_code, error_msg);
            return json_response(http::status::bad_request, response_body, ContentType::APP_JSON);
        }

        template <typename Handler>
        static StringResponse MakeNotFoundResponse(const Handler& json_response, 
                                                   std::string_view error_code = "",
                                                   std::string_view error_msg = "") {
            std::string response_body = 
                error_code.empty() ? NotFound() : NotFound(error_code, error_msg);
            return json_response(http::status::not_found, response_body, ContentType::APP_JSON);
        }
    };

    class HttpResponse {
    public:
        static void MakeResponse(StringResponse& response, std::string body,
                                 bool keep_alive,
                                 std::string_view content_type = ContentType::TEXT_HTML);

        static StringResponse MakeStringResponse(http::status status, std::string body_sv,
                                                 unsigned http_version, bool keep_alive,
                                                 std::string_view content_type = ContentType::TEXT_HTML);
    };

    class ApiRequestHandler {
    public:
        ApiRequestHandler(model::Game& game, fs::path path);

        template <typename Handler>
        StringResponse GameRequest(const StringRequest& request, 
                                   const Handler& json_response,
                                   const std::vector<std::string>& path_components);
        
        template <typename Handler>
        StringResponse MapRequest(const StringRequest& request, 
                                   Handler&& json_response,
                                   const std::vector<std::string>& path_components);

        template <typename Handler>
        StringResponse HandleRequest(StringRequest&& request, const Handler& json_response);

        template <typename Handler>
        StringResponse GetMapsRequest(const Handler& json_response) const;
        template <typename Handler>
        StringResponse GetMapDetailsRequest(std::string_view map_id, 
                                                  const Handler& json_response) const;

    private:
        model::Game& game_;
        fs::path root_dir_;
    };

    class FileRequestHandler {
    public:
        FileRequestHandler(model::Game& game, fs::path path);

        template <typename Handler>
        ResponseVariant HandleRequest(const StringRequest& request, const Handler& json_response);

    private:
        model::Game& game_;
        fs::path root_dir_;
    };

    class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
    public:
        using Strand = net::strand<net::io_context::executor_type>;

        RequestHandler(model::Game& game, Strand api_strand, fs::path path);

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        ResponseVariant HandleRequest(StringRequest&& req);

        template <typename Body, typename Allocator, typename Send>
        EmptyResponse operator()(http::request<Body, 
                                                    http::basic_fields<Allocator>>&& req, 
                                                    Send&& send) {
            ResponseVariant response = HandleRequest(std::move(req));
            
            auto response_copy = CopyResponseWithoutBody(response);
            std::visit([&send](auto&& result){
                send(std::forward<decltype(result)>(result));
            }, std::move(response));

            return response_copy;
        }

    private:
        model::Game& game_;
        fs::path root_dir_;
        std::unique_ptr<router::Router> router_;

        FileRequestHandler file_handler_{game_, root_dir_};

        ApiRequestHandler api_handler_{game_, root_dir_};
        Strand api_strand_;

        void SetupRoutes() {
            using JsonResponseHandler = 
                std::function<StringResponse(http::status, std::string, std::string_view)>;

            router_->AddRoute({"GET"}, "/api/v1/maps", 
                std::make_unique<HTTPResponseMaker>(
                [this](const StringRequest& req, JsonResponseHandler json_response) -> ResponseVariant {
                    url::UrlParser parser(std::string(req.target()));
                    auto path_components = parser.GetComponents();
                    return this->api_handler_.MapRequest(req, json_response, path_components);
                }
            ));

            router_->AddRoute({"GET"}, "/api/v1/maps/:id", 
                std::make_unique<HTTPResponseMaker>(
                [this](const StringRequest& req, JsonResponseHandler json_response) -> ResponseVariant {
                    url::UrlParser parser(std::string(req.target()));
                    auto map_id = parser.GetLastComponent();
                    return this->api_handler_.GetMapDetailsRequest(map_id, json_response);
                }
            ));

            router_->AddRoute({"GET"}, ":", 
                std::make_unique<HTTPResponseMaker>(
                [this](const StringRequest& req, JsonResponseHandler json_response) -> ResponseVariant {
                    return this->file_handler_.HandleRequest(req, json_response);
                }
            ));



            

        //     router_.AddRoute({"GET"}, "/api/v1/maps/map1", Map1Handler);
        //     router_.AddRoute({"GET"}, "/game.html", GameHandler);
        }

        http::response<http::empty_body> CopyResponseWithoutBody(const ResponseVariant& response) const;
    };

    template<class SomeRequestHandler>
    class LoggingRequestHandler {
        template <typename Body, typename Allocator>
        static void LogRequest(http::request<Body, http::basic_fields<Allocator>>& req);
        static void LogResponse(http::response<http::empty_body>& response, int resp_duration);

    public:
        LoggingRequestHandler(SomeRequestHandler& handler) : request_handler_(handler) {};

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            LogRequest(req);
            auto t1 = clock();
            EmptyResponse response = request_handler_(std::move(req), std::forward<Send>(send));
            auto t2 = clock();
            LogResponse(response, int(t2 - t1));
        }

    private:
        SomeRequestHandler& request_handler_;
    };
}  // namespace http_handler

namespace http_handler {

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
            json::value jv;
            jv = {
                {"response_time", resp_duration},
                {"code", response.result_int()},
                {"content_type", response.base()["Content-Type"]}
            };

            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, jv) << "response sent";
        }

    // Template function definitions
    template <typename Handler>
    StringResponse ApiRequestHandler::GetMapsRequest(const Handler& json_response) const {
        std::string maps = json_loader::MapSerializer::SerializeMapsMainInfo(game_.GetMaps());

        return json_response(http::status::ok, maps, ContentType::APP_JSON);
    }

    template <typename Handler>
    StringResponse ApiRequestHandler::GetMapDetailsRequest(std::string_view map_id, 
                                                           const Handler& json_response) const {
        const model::Map::Id id{std::string(map_id)};
        auto map_ptr = game_.FindMap(id);
        
        if (map_ptr) {
            auto map_json = json_loader::MapSerializer::SerializeSingleMap(*map_ptr);
            std::string serialized_map = boost::json::serialize(std::move(map_json));
            return json_response(http::status::ok, serialized_map, ContentType::APP_JSON);
        }

        return ErrorHandler::MakeNotFoundResponse(json_response, "mapNotFound", "Map not found");
    }

    template <typename Handler>
    StringResponse ApiRequestHandler::GameRequest(const StringRequest& request, 
                                const Handler& json_response,
                                const std::vector<std::string>& path_components) {
        if (path_components[2] == "game") {
            if (path_components.size() == 3) { 
                return GetMapsRequest(json_response);
            } else if (path_components.size() == 4) {
                return GetMapDetailsRequest(path_components[3], json_response);
            }
        }

        return ErrorHandler::MakeBadRequestResponse(json_response);
    }
    
    template <typename Handler>
    StringResponse ApiRequestHandler::MapRequest(const StringRequest& request, 
                                Handler&& json_response,
                                const std::vector<std::string>& path_components) {
        if (path_components.size() == 3) { 
            return GetMapsRequest(json_response);
        } else if (path_components.size() == 4) {
            return GetMapDetailsRequest(path_components[3], json_response);
        }

        return ErrorHandler::MakeBadRequestResponse(json_response);
    }

    template <typename Handler>
    StringResponse ApiRequestHandler::HandleRequest(StringRequest&& request, 
                                                    const Handler& json_response) {
        url::UrlParser parser(util::UrlDecode(std::string(request.target())));
        auto path_components = parser.GetComponents();

        if (path_components.size() >= 3 && path_components[0] == "api" && 
            path_components[1] == "v1") {
                if (path_components[2] == "maps") {
                    return MapRequest(std::move(request), json_response, path_components);
                } else if (path_components[2] == "game") {
                    return GameRequest(std::move(request), json_response, path_components); 
                }
        }

        return ErrorHandler::MakeBadRequestResponse(json_response);
    }

        template <typename Handler>
        ResponseVariant FileRequestHandler::HandleRequest(const StringRequest& request, 
                                                          const Handler& json_response) {
            fs::path base_path = fs::weakly_canonical(root_dir_.c_str());
            fs::path abs_path = ProcessingAbsPath(root_dir_.c_str(), request.target());

            if (IsSubPath(abs_path, base_path)) {
                if (fs::exists(abs_path)) {
                    return util::ReadStaticFile(abs_path);
                }

                return json_response(http::status::not_found, 
                                     ErrorHandler::NotFound("fileNotFound", "File not found"),
                                     ContentType::TEXT_PLAIN);
            }

            return ErrorHandler::MakeBadRequestResponse(json_response);
        }
}
