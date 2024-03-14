#pragma once
#include "http_server.h"
#include "model.h"
#include <string_view>
#include <boost/json.hpp>

namespace http_handler {

    using namespace std::literals;
    namespace beast = boost::beast;
    namespace http = beast::http;
    
    // ========================================================
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
        static boost::json::object BadRequest(const std::string& error_message) {
            boost::json::object obj;
            obj["error"] = error_message;
            obj["code"] = 400;
            return obj;
        }

        // Можно добавить другие методы для создания специфичных JSON-ответов
    };

    class HttpResponse {
    public:
        static StringResponse MakeResponse(StringResponse& response, std::string_view body,
                                        bool keep_alive,
                                        std::string_view content_type = ContentType::TEXT_HTML) {
            response.set(http::field::content_type, content_type);
            response.body() = body;
            response.content_length(body.size());
            response.keep_alive(keep_alive);
            return response;
        }

        static StringResponse CreateStringResponse(http::status status,
                                                const boost::json::value& jsonBody,
                                                unsigned http_version,
                                                bool keep_alive,
                                                std::string_view content_type = ContentType::TEXT_HTML) {
            std::string body = boost::json::serialize(jsonBody);
            StringResponse response(status, http_version);
            MakeResponse(response, body, keep_alive, content_type);
            if (status == http::status::method_not_allowed) {
                BadRequest(response);
            }
            return response;
        }

        // Методы для создания других типов ответов
    };

    // Создает ответ на неверный запрос
    StringResponse BadRequest(StringResponse& response) {    
        response.set(http::field::allow, "GET, HEAD"sv);
        // response.result
        return response;
    }

    /* // Заполняет ответ на запрос
    StringResponse MakeResponse(StringResponse& response, std::string_view body,
                                    bool keep_alive,
                                    std::string_view content_type = ContentType::TEXT_HTML) {
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }
    */
   
    // Создаёт StringResponse с заданными параметрами
    StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                    bool keep_alive,
                                    std::string_view content_type = ContentType::TEXT_HTML) {
        StringResponse response(status, http_version);
        MakeResponse(response, body, keep_alive, content_type);
        if (status == http::status::method_not_allowed) {
            BadRequest(response);
        }
        return response;
    }

    std::string GenerateResponseBody(StringRequest& req) {
        const std::string_view target = req.target();
        std::string response;
        if (req.method() == http::verb::head) {
            return ;
        }

        response.append("<strong>"s);
        response.append("Hello, "s + std::string(target.substr(target.find_last_of('/') + 1, std::string_view::npos)));
        response.append("</strong>"s);
        return response;
    }

    StringResponse HandleRequest(StringRequest&& req) {
        /* const auto text_response = [&req](http::status status, std::string_view text) {
            return MakeStringResponse(status, text, req.version(), req.keep_alive());
        };

        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            return text_response(http::status::method_not_allowed, "Invalid method"sv);
        } */

        if (req.target() == "/api/v1/maps") {
            
        }
        
        return MakeStringResponse(http::status::ok, GenerateResponseBody(req), req.version(), req.keep_alive());
    }
// ========================================================
    class RequestHandler {
    public:
        explicit RequestHandler(model::Game& game)
            : game_{game} {
        }

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            
            
        }

    private:
        model::Game& game_;
    };

}  // namespace http_handler
    