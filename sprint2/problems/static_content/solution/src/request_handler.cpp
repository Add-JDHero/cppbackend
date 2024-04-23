#include "request_handler.h"

#include "boost/beast/core/string_type.hpp"
#include <variant>

namespace http_handler {
    
    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки-fdiagnostics-color=always
    using StringResponse = http::response<http::string_body>;

    bool IsAllowedReqMethod(beast::http::verb method) {
        return method == http::verb::get || method != http::verb::head;
    }

    fs::path ProcessingAbsPath(std::string_view base, std::string_view rel) {
        fs::path base_path = fs::weakly_canonical(base);
        fs::path rel_path{fs::weakly_canonical(std::string(rel))};
        rel_path = rel_path.relative_path();
        if (rel_path.empty() || (!rel_path.empty() && rel_path.string().back() == separating_chars::SLASH)) {
            rel_path.append("index.html");
        }

        return fs::weakly_canonical(base_path / rel_path);
    }

    bool IsSubPath(fs::path path, fs::path base) {
        // Приводим оба пути к каноничному виду (без . и ..)
        path = fs::weakly_canonical(path);
        base = fs::weakly_canonical(base);

        // Проверяем, что все компоненты base содержатся внутри path
        for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
            if (p == path.end() || *p != *b) {
                return false;
            }
        }
        return true;
    }


    std::string JsonResponseBuilder::BadRequest(std::string_view code, std::string_view error_message) {
        boost::json::object obj;
        obj["error"] = std::string(error_message);
        obj["code"] = code.data();

        return boost::json::serialize(obj);
    }

    std::string JsonResponseBuilder::NotFound(std::string_view code, std::string_view error_message) {
        boost::json::object obj;
        obj["error"] = std::string(error_message);
        obj["code"] = code.data();

        return boost::json::serialize(obj);
    }

    void HttpResponse::MakeResponse(StringResponse& response, std::string body,
                                            bool keep_alive,
                                            std::string_view content_type) {
        response.set(http::field::content_type, content_type);
        response.body() = std::move(std::string(body));
        response.content_length(body.size());
        response.keep_alive(keep_alive);
    }

    StringResponse HttpResponse::MakeStringResponse(http::status status, std::string body_sv,
                                                unsigned http_version, bool keep_alive,
                                                std::string_view content_type) {
        StringResponse response(status, http_version);
        MakeResponse(response, body_sv, keep_alive, content_type);  // Уже не нужно копировать body_sv
        return response;
    }

    std::string ParseMapStringRequest(std::string_view sv) {
        sv.remove_prefix(sv.find_last_of('/') + 1);
        
        return std::string(sv);
    }

    ResponseVariant RequestHandler::HandleRequest(StringRequest&& req) {
        auto json_response = [this, version = req.version(), keep_alive = req.keep_alive()]
                             (http::status status, 
                              std::string body = {},
                              std::string_view content_type = ContentType::APP_JSON) {
            return HttpResponse::MakeStringResponse(status, 
                body, 
                version, 
                keep_alive, 
                content_type);
        };

        if (!IsAllowedReqMethod(req.method())) {
            return json_response(http::status::method_not_allowed);
        }

        return ProcessRequest(req.target(), json_response);
    }

    RequestHandler::RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler::RequestHandler(model::Game& game, const char* root_dir)
        : game_{game}, root_dir_(root_dir){
    }

}  // namespace http_handler