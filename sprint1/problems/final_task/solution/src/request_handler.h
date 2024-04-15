#pragma once
#include "http_server.h"
#include "json_loader.h"
#include "model.h"
#include <boost/json/serialize.hpp>
#include <string_view>
#include <unordered_map>
#include <boost/json.hpp>

namespace http_handler {

    using namespace std::literals;
    namespace beast = boost::beast;
    namespace http = beast::http;
    
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
        explicit RequestHandler(model::Game& game);

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        StringResponse HandleRequest(StringRequest&& req);

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            send(HandleRequest(std::move(req)));
        }

    private:
        model::Game& game_;

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
        StringResponse HandleBadRequest(Handler&& json_response) const {
            return json_response(http::status::bad_request, JsonResponseBuilder::BadRequest());
        }
    };

}  // namespace http_handler
