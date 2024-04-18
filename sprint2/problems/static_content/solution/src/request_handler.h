#pragma once

#include "http_server.h"
#include "json_loader.h"
#include "model.h"
#include "url_parser.h"
#include "util.h"

#include <boost/json/serialize.hpp>
#include <string_view>
#include <unordered_map>
#include <filesystem>
#include <boost/json.hpp>

namespace http_handler {

    using namespace std::literals;
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace fs = std::filesystem;

    // Из примеров кода boost::beast
    beast::string_view MimeType(beast::string_view path);

    bool IsSubPath(fs::path path, fs::path base);
    
    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;

    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        constexpr static std::string_view APP_JSON = "application/json"sv;
        // При необходимости внутрь ContentType можно добавить и другие типы контента
    };

    class JsonResponseBuilder {
    public:
        static std::string BadRequest(std::string_view error_message = "Bad Request");

        static std::string NotFound(std::string_view error_message = "Map not found");
    };

    class HttpResponse {
    public:
        static StringResponse MakeResponse(StringResponse& response, std::string_view body,
                                        bool keep_alive,
                                        std::string_view content_type = ContentType::TEXT_HTML);

        static StringResponse MakeStringResponse(http::status status,
                                                std::string_view body_sv,
                                                unsigned http_version,
                                                bool keep_alive,
                                                std::string_view content_type = ContentType::TEXT_HTML);
    };

    std::string GenerateResponseBody(StringRequest& req, const model::Game& game);

    // StringResponse HandleRequest(StringRequest&& req, const model::Game& game);


    class RequestHandler {
    public:

        using HandlerFunction = std::function<StringResponse()>;

        explicit RequestHandler(model::Game& game);
        RequestHandler(model::Game& game, const char* root_dir);

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        StringResponse HandleRequest(StringRequest&& req);

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            send(HandleRequest(std::move(req)));
        }

    private:
        model::Game& game_;
        const char* root_dir_;

        template <typename Handler>
        HandlerFunction HandleApiRequest(const std::vector<std::string>& path_components, Handler&& json_response) {
            if (path_components.size() >= 3 && path_components[0] == "api" && path_components[1] == "v1" && path_components[2] == "maps") {
                if (path_components.size() == 3) {
                    return [this, &json_response]() { return HandleGetMapsRequest(json_response); };
                } else if (path_components.size() == 4) {
                    return [this, &json_response, map_id = path_components[3]]() { return HandleGetMapDetailsRequest(json_response, map_id); };
                }
            }

            return [this, &json_response]() { return HandleBadRequest(json_response); };
        }

        template <typename Handler>
        HandlerFunction HandleGetFileRequest(std::string_view path, Handler&& json_response) {
            fs::path base_path = fs::weakly_canonical(root_dir_);
            if (!path.empty() && path[0] == '/') {
                path.remove_prefix(1);
            }
            fs::path rel_path{std::string(path)};
            std::string_view file_name = path.substr(path.find_last_of('/') + 1, std::string_view::npos);
            fs::path abs_path = fs::weakly_canonical(base_path / rel_path);

            if (IsSubPath(abs_path, base_path)) {
                return [this, json_response, abs_path]() { 
                    return json_response(http::status::ok, util::ReadFromFileIntoString(abs_path));
                };
            }

            return [this, &json_response]() { return HandleBadRequest(json_response); };
        }

        template <typename Handler>
        HandlerFunction DetermineHandler(std::string_view path, Handler&& json_response) {
            url::UrlParser parser( (std::string(path)) );
            auto path_components = parser.GetComponents();

            if (path_components.size() > 0 && path_components[0] == "api"sv) {
                return HandleApiRequest(path_components, json_response);
            }

            return HandleGetFileRequest(path.data(), json_response);
        }

        template <typename Handler>
        StringResponse HandleGetMapsRequest(Handler&& json_response) const {
            std::string maps = json_loader::MapSerializer::SerializeMapsMainInfo(game_.GetMaps());
            StringResponse response = json_response(http::status::ok, maps);

            return response;
        }

        template <typename Handler>
        StringResponse HandleGetMapDetailsRequest(Handler&& json_response, std::string_view map_id) const {
            StringResponse response;
            const model::Map::Id id{std::string(map_id)};
            auto map_ptr = game_.FindMap(id);
            
            if (map_ptr) {
                std::string map = boost::json::serialize(json_loader::MapSerializer::SerializeSingleMap(*map_ptr));
                return response = json_response(http::status::ok, map);
            }

            return response = json_response(http::status::not_found, JsonResponseBuilder::NotFound());
        }

        template <typename Handler>
        static StringResponse HandleBadRequest(Handler&& json_response) {
            return json_response(http::status::bad_request, JsonResponseBuilder::BadRequest());
        }

        bool IsAllowedReqMethod(const StringRequest& req) {
            return req.method() == http::verb::get || req.method() != http::verb::head;
        }
    };

}  // namespace http_handler