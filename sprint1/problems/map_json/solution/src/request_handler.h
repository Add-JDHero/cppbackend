#pragma once
#include "http_server.h"
#include "json_loader.h"
#include "model.h"
#include <string_view>
#include <unordered_map>
#include <boost/json.hpp>

namespace http_handler {

    using namespace std::literals;
    namespace beast = boost::beast;
    namespace http = beast::http;
    
    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки-fdiagnostics-color=always
    using StringResponse = http::response<http::string_body>;

    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        constexpr static std::string_view APP_JSON = "application/json"sv;
        // При необходимости внутрь ContentType можно добавить и другие типы контента
    };

    class JsonResponseBuilder {
    public:
        static boost::json::object BadRequest(std::string_view error_message);

        static boost::json::object NotFound(std::string_view error_message);
    };

    // TODO: 
    /* class UrlResolver {
    public:

    private:
        std::unordered_map<std::string, Handler> url_resolver_;
    }; */

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

    StringResponse HandleRequest(StringRequest&& req, const model::Game& game);


    class RequestHandler {
    public:
        explicit RequestHandler(model::Game& game);

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            send(HandleRequest(std::move(req), game_));
        }

    private:
        model::Game& game_;
    };

}  // namespace http_handler
    