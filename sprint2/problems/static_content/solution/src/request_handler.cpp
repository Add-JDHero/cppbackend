#include "request_handler.h"
#include "boost/beast/core/string_type.hpp"
#include <variant>

namespace http_handler {
    namespace sys = boost::system;
    

    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки-fdiagnostics-color=always
    using StringResponse = http::response<http::string_body>;

    fs::path ProcessingAbsPath(std::string_view base, std::string_view rel) {
        fs::path base_path = fs::weakly_canonical(base);
        fs::path rel_path{fs::weakly_canonical(std::string(rel))};
        rel_path = rel_path.relative_path();
        if (rel_path.empty() || (!rel_path.empty() && rel_path.string().back() == separating_chars::SLASH)) {
            rel_path.append("index.html");
        }

        return fs::weakly_canonical(base_path / rel_path);
    }
    
    // fs::path ProcessingAbsPath(std::string_view base, std::string_view rel) {
    //     fs::path base_path = fs::canonical(base); // Используйте canonical вместо weakly_canonical
    //     fs::path rel_path(rel);
    //     if (!rel_path.is_absolute()) {
    //         rel_path = base_path / rel_path;
    //     }
    //     return fs::canonical(rel_path); // Убедитесь, что путь существует и корректен
    // }

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

    StringResponse HttpResponse::MakeResponse(StringResponse& response, std::string_view body,
                                            bool keep_alive,
                                            std::string_view content_type) {
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);

        return response;
    }

    http::response<http::file_body> ReadStaticFile(std::string_view file_name) {
        http::response<http::file_body> res;
        res.version(11);  // HTTP/1.1
        res.result(http::status::ok);
        res.insert(http::field::content_type, "text/plain"sv);

        http::file_body::value_type file;

        if (sys::error_code ec; file.open(file_name.data(), beast::file_mode::read, ec), ec) {
            std::cout << "Failed to open file "sv << file_name << std::endl;
        }

        return {};
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

    ResponseVariant RequestHandler::HandleRequest(StringRequest&& req) {
        const auto json_response = [&req](http::status status, std::string_view body = {}) {
            return HttpResponse::MakeStringResponse(status, 
                   body, 
                   req.version(), 
                   req.keep_alive(), 
                   ContentType::APP_JSON);
        };

        if (!IsAllowedReqMethod(req)) {
            return json_response(http::status::method_not_allowed);
        }

        std::string path = std::string(req.target());
        auto response = ProcessRequest(path, json_response);
        return response;
    }

    RequestHandler::RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler::RequestHandler(model::Game& game, const char* root_dir)
        : game_{game}, root_dir_(root_dir){
    }

}  // namespace http_handler