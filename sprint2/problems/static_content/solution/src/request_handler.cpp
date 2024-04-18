#include "request_handler.h"
#include "boost/beast/core/string_type.hpp"

namespace http_handler {
    namespace sys = boost::system;
    

    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки-fdiagnostics-color=always
    using StringResponse = http::response<http::string_body>;

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

    beast::string_view MimeType(beast::string_view path) {
        using beast::iequals;
        auto const ext = [&path]{
            auto const pos = path.rfind(".");
            if(pos == beast::string_view::npos)
                return beast::string_view{};
            return path.substr(pos);
        }();
        if (iequals(ext, ".htm"))  return "text/html";
        if (iequals(ext, ".html")) return "text/html";
        // if(iequals(ext, ".php"))  return "text/html";
        if (iequals(ext, ".css"))  return "text/css";
        if (iequals(ext, ".txt"))  return "text/plain";
        if (iequals(ext, ".js"))   return "text/javascript";
        if (iequals(ext, ".json")) return "application/json";
        if (iequals(ext, ".xml"))  return "application/xml";
        // if (iequals(ext, ".swf"))  return "application/x-shockwave-flash";
        // if (iequals(ext, ".flv"))  return "video/x-flv";
        if (iequals(ext, ".png"))  return "image/png";
        if (iequals(ext, ".jpe"))  return "image/jpeg";
        if (iequals(ext, ".jpeg")) return "image/jpeg";
        if (iequals(ext, ".jpg"))  return "image/jpeg";
        if (iequals(ext, ".gif"))  return "image/gif";
        if (iequals(ext, ".bmp"))  return "image/bmp";
        if (iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
        if (iequals(ext, ".tiff")) return "image/tiff";
        if (iequals(ext, ".tif"))  return "image/tiff";
        if (iequals(ext, ".svg"))  return "image/svg+xml";
        if (iequals(ext, ".svgz")) return "image/svg+xml";
        if (iequals(ext, ".mp3")) return "audio/mpeg";
        return "application/octet-stream";
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

    std::string ExtractFileExtension(std::string_view path) {
        if (path.find_last_of('.') != std::string_view::npos) {
            beast::string_view ext = path.substr(path.find_last_of('.'), beast::string_view::npos);
            return std::string(ext);
        }
        
        return {};
    }

    StringResponse RequestHandler::HandleRequest(StringRequest&& req) {
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

        auto handler = DetermineHandler(req.target(), json_response);
        auto result = handler();

        result.insert(http::field::content_type, MimeType(ExtractFileExtension(req.target())));

        return result;
    }

    RequestHandler::RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler::RequestHandler(model::Game& game, const char* root_dir)
        : game_{game}, root_dir_(root_dir){
    }

}  // namespace http_handler