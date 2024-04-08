#include "request_handler.h"
#include "url_parser.h"

namespace http_handler {

    using namespace std::literals;
    namespace beast = boost::beast;
    namespace http = beast::http;
    

    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки-fdiagnostics-color=always
    using StringResponse = http::response<http::string_body>;

    // JsonResponseBuilder realizations
    std::string JsonResponseBuilder::BadRequest(std::string_view error_message) {
        boost::json::object obj;
        obj["error"] = std::string(error_message);
        obj["code"] = "badRequest";
        return boost::json::serialize(obj);
    }

    std::string JsonResponseBuilder::NotFound(std::string_view error_message) {
        boost::json::object obj;
        obj["error"] = std::string(error_message);
        obj["code"] = "mapNotFound";
        return boost::json::serialize(obj);
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

    std::string GenerateResponseBody(StringRequest& req, const model::Game& game) {
        const std::string_view target = req.target();
        std::string response;
        
        // const std::shared_ptr<model::Map> map = nullptr;

        return response;
    }

    StringResponse RequestHandler::HandleRequest(StringRequest&& req) {
        const auto json_response = [&req](http::status status, std::string_view body = {}) {
            return HttpResponse::MakeStringResponse(status, 
                body, 
                req.version(), 
                req.keep_alive(), 
                ContentType::APP_JSON);
        };

        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            return json_response(http::status::method_not_allowed);
        }

        url::UrlParser parser(std::string(req.target()));
        auto path_components = parser.getComponents();

        if (path_components.size() >= 3 && path_components[0] == "api"sv && path_components[1] == "v1"sv && path_components[2] == "maps"sv) {
            if (path_components.size() == 3) {
                return HandleGetMapsRequest(json_response);
            } else if (path_components.size() == 4) {
                return HandleGetMapDetailsRequest(json_response, path_components[3]);
            } else {
                return HandleBadRequest(json_response);
            }
        }

        return HandleBadRequest(json_response);
    }

    RequestHandler::RequestHandler(model::Game& game)
        : game_{game} {
    }

}  // namespace http_handler
