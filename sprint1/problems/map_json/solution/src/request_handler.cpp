#include "request_handler.h"

namespace http_handler {

    using namespace std::literals;
    namespace beast = boost::beast;
    namespace http = beast::http;
    
    // ========================================================
    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки-fdiagnostics-color=always
    using StringResponse = http::response<http::string_body>;

    // JsonResponseBuilder realizations
    boost::json::object JsonResponseBuilder::BadRequest(std::string_view error_message) {
        boost::json::object obj;
        obj["error"] = std::string(error_message);
        obj["code"] = "badRequest";
        return obj;
    }

    boost::json::object JsonResponseBuilder::NotFound(std::string_view error_message) {
        boost::json::object obj;
        obj["error"] = std::string(error_message);
        obj["code"] = "mapNotFound";
        return obj;
    }

    // HttpResponse realizations
    StringResponse HttpResponse::MakeResponse(StringResponse& response, std::string_view body,
                                    bool keep_alive,
                                    std::string_view content_type) {
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    StringResponse HttpResponse::MakeStringResponse(http::status status,
                                            std::string_view body_sv,
                                            unsigned http_version,
                                            bool keep_alive,
                                            std::string_view content_type) {
        std::string body = std::string(body_sv);
        StringResponse response(status, http_version);
        MakeResponse(response, body, keep_alive, content_type);
        return response;
    }

    std::string ParseMapStringRequest(std::string_view sv) {
        sv.remove_prefix(sv.find_last_of('/') + 1);
        
        return std::string(sv);
    }

    // Other functions realisation
    std::string GenerateResponseBody(StringRequest& req, const model::Game& game) {
        const std::string_view target = req.target();
        std::string response;
        
        const model::Map* map = nullptr;

        if (req.target().find("/api/v1/") != std::string_view::npos) {
            response = json_loader::MapSerializer::SerializeMapsMainInfo(game.GetMaps());
        } else {
            
        }

        return response;
    }

    StringResponse HandleRequest(StringRequest&& req, const model::Game& game) {
        const auto json_response = [&req](http::status status, std::string_view body = {}) {
            return HttpResponse::MakeStringResponse(status, body, req.version(), req.keep_alive(), ContentType::APP_JSON);
        };

        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            return json_response(http::status::method_not_allowed);
        }

        return json_response(http::status::ok, GenerateResponseBody(req, game));
    }

    RequestHandler::RequestHandler(model::Game& game)
        : game_{game} {
    }

}  // namespace http_handler
