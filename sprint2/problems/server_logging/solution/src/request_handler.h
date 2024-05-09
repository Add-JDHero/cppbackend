#pragma once

#include "sdk.h" 
#include "http_server.h"
#include "json_loader.h"
#include "model.h"
#include "url_parser.h"
#include "util.h"
#include "log.h"

#include <boost/json/serialize.hpp>
#include <string_view>
#include <unordered_map>
#include <filesystem>
#include <ctime>
#include <variant>
#include <boost/json.hpp>
#include <cassert>

namespace http_handler {

    using namespace std::literals;
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;
    namespace fs = std::filesystem;

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
    using ResponseVariant = std::variant<StringResponse, FileResponse>;

    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
        constexpr static std::string_view APP_JSON = "application/json"sv;
        // При необходимости внутрь ContentType можно добавить и другие типы контента
    };

    class JsonResponseBuilder {
    public:
        static std::string BadRequest(std::string_view code = "badRequest", 
                                      std::string_view error_message = "Bad Request");

        static std::string NotFound(std::string_view code = "notFound",
                                    std::string_view error_message = "Not found");
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

    class RequestHandler {
    public:
        explicit RequestHandler(model::Game& game);
        RequestHandler(model::Game& game, const char* root_dir);

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        ResponseVariant HandleRequest(StringRequest&& req);

        template <typename Body, typename Allocator, typename Send>
        http::response<http::empty_body> operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            ResponseVariant response = HandleRequest(std::move(req));
            http::response<http::empty_body> response_copy = CopyResponseWithoutBody(response);
            if (holds_alternative<StringResponse>(response)) {
                auto& string_response = std::get<StringResponse>(response);
                send(std::move(string_response));
            } else if (holds_alternative<FileResponse>(response)) {
                auto& file_response = std::get<FileResponse>(response);
                send(std::move(file_response));
            }

            return response_copy;
        }

    private:
        model::Game& game_;
        const char* root_dir_;

        http::response<http::empty_body> CopyResponseWithoutBody(const ResponseVariant& response) const;

        template <typename Handler>
        StringResponse HandleApiRequest(const std::vector<std::string>& path_components, 
                                        const Handler& json_response);

        template <typename Handler>
        ResponseVariant HandleGetFileRequest(std::string_view req_path, const Handler& json_response);

        template <typename Handler>
        ResponseVariant ProcessRequest(std::string_view path, const Handler& json_response);

        template <typename Handler>
        StringResponse HandleGetMapsRequest(const Handler& json_response) const;
        template <typename Handler>
        StringResponse HandleGetMapDetailsRequest(std::string_view map_id, 
                                                  const Handler& json_response) const;

        template <typename Handler>
        static StringResponse HandleBadRequest(const Handler& json_response, std::string_view error_code = {});

        template <typename Handler>
        static StringResponse HandleNotFound(const Handler& json_response, 
                                             std::string_view error_code = {}, std::string_view error_msg = {});
    };

    template<class SomeRequestHandler>
    class LoggingRequestHandler {
        template <typename Body, typename Allocator>
        static void LogRequest(http::request<Body, http::basic_fields<Allocator>>& req) {
            boost::json::value jv = {
                {"URI", static_cast<std::string>(req.target())},
                {"method", req.method_string()}
            };
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, jv) << "request received";
        }

        static void LogResponse(http::response<http::empty_body>& response, int resp_duration) {
            json::value jv;
            jv = {
                {"response_time", resp_duration},
                {"code", response.result_int()},
                {"content_type", response.base()["Content-Type"]}
                };
            

            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, jv) << "response sent";
        }

    public:
        LoggingRequestHandler(SomeRequestHandler& handler) : request_handler_(handler) {};

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            LogRequest(req);
            auto t1 = clock();
            http::response<http::empty_body> response = request_handler_(std::move(req), std::forward<Send>(send));
            auto t2 = clock();
            LogResponse(response, int(t2 - t1));
        }

    private:
        SomeRequestHandler& request_handler_;
    };


    // Template function definitions

    template <typename Handler>
    StringResponse RequestHandler::HandleApiRequest(const std::vector<std::string>& path_components, 
                                    const Handler& json_response) {
        if (path_components.size() >= 3 && path_components[0] == "api" && 
            path_components[1] == "v1" && path_components[2] == "maps") {

            if (path_components.size() == 3) { 
                return HandleGetMapsRequest(json_response);
            } else if (path_components.size() == 4) {
                return HandleGetMapDetailsRequest(path_components[3], json_response);
            }
        }

        return HandleBadRequest(json_response);
    }

    template <typename Handler>
    ResponseVariant RequestHandler::HandleGetFileRequest(std::string_view req_path, const Handler& json_response) {
        fs::path base_path = fs::weakly_canonical(root_dir_);
        fs::path abs_path = ProcessingAbsPath(root_dir_, req_path);

        std::cout << abs_path.c_str() << std::endl;

        if (IsSubPath(abs_path, base_path)) {
            if (fs::exists(abs_path)) {
                return util::ReadStaticFile(abs_path);
            }

            return json_response(http::status::not_found, 
                                    JsonResponseBuilder::NotFound("fileNotFound", "File not found"),
                                    ContentType::TEXT_PLAIN);
        }

        return HandleBadRequest(json_response);
    }

    template <typename Handler>
    ResponseVariant RequestHandler::ProcessRequest(std::string_view path, const Handler& json_response) {
        std::string decoded_path = util::UrlDecode(std::string(path));
        url::UrlParser parser(decoded_path);
        auto path_components = parser.GetComponents();

        if (path_components.size() > 0 && path_components[0] == "api"sv) {
            return HandleApiRequest(path_components, json_response);
        }

        return HandleGetFileRequest(decoded_path, json_response);
    }

    template <typename Handler>
    StringResponse RequestHandler::HandleGetMapsRequest(const Handler& json_response) const {
        std::string maps = json_loader::MapSerializer::SerializeMapsMainInfo(game_.GetMaps());

        return json_response(http::status::ok, maps);
    }

    template <typename Handler>
    StringResponse RequestHandler::HandleGetMapDetailsRequest(std::string_view map_id, 
                                                const Handler& json_response) const {
        const model::Map::Id id{std::string(map_id)};
        auto map_ptr = game_.FindMap(id);
        
        if (map_ptr) {
            auto map_json = json_loader::MapSerializer::SerializeSingleMap(*map_ptr);
            std::string serialized_map = boost::json::serialize(std::move(map_json));
            return json_response(http::status::ok, serialized_map);
        }

        return HandleNotFound(json_response, "mapNotFound", "Map not found");
    }

    template <typename Handler>
    StringResponse RequestHandler::HandleBadRequest(const Handler& json_response, std::string_view error_code) {
        bool has_error_code = error_code.size() > 0;
        std::string ans_body = has_error_code 
            ? JsonResponseBuilder::BadRequest(error_code) 
            : JsonResponseBuilder::BadRequest();
        return json_response(http::status::bad_request, ans_body);
    }

    template <typename Handler>
    StringResponse RequestHandler::HandleNotFound(const Handler& json_response, 
                                            std::string_view error_code, std::string_view error_msg) {
        bool has_error_code = error_code.size() > 0;
        std::string ans_body = has_error_code 
            ? JsonResponseBuilder::NotFound(error_code) 
            : JsonResponseBuilder::NotFound();
        return json_response(http::status::not_found, ans_body);
    }

}  // namespace http_handler
