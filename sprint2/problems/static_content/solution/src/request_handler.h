#pragma once

#include "boost/utility/string_view.hpp"
#include "boost/utility/string_view_fwd.hpp"
#include "http_server.h"
#include "json_loader.h"
#include "model.h"
#include "url_parser.h"
#include "util.h"

#include <boost/json/serialize.hpp>
#include <string_view>
#include <unordered_map>
#include <filesystem>
#include <variant>
#include <boost/json.hpp>
#include <cassert>

#define BOOST_BEAST_USE_STD_STRING_VIEW


namespace http_handler {

    using namespace std::literals;
    namespace beast = boost::beast;
    namespace http = beast::http;
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
        static void MakeResponse(StringResponse& response, std::string body,
                                            bool keep_alive,
                                            std::string_view content_type = ContentType::TEXT_HTML);

        static StringResponse MakeStringResponse(http::status status, std::string body_sv,
                                                unsigned http_version, bool keep_alive,
                                                std::string_view content_type = ContentType::TEXT_HTML);
    };

    // // Абстрактный базовый класс для обработки ответов
    // class ResponseInterface {
    // public:
    //     virtual ~ResponseInterface() = default;

    //     // Виртуальный метод для выполнения каких-либо действий с ответом
    //     virtual void ProcessResponse() = 0;
    //     virtual void PrepareResponse() = 0;

    //     virtual std::shared_ptr<void> get() = 0;
    // };

    // class StringResponseWrapper : public ResponseInterface {
    //     std::shared_ptr<http::response<http::string_body>> response_;

    // public:
    //     StringResponseWrapper(http::response<http::string_body> response) 
    //         : response_(std::make_shared<http::response<http::string_body>>(std::move(response))) {
    //     }

    //     void ProcessResponse() override {
    //     }

    //     void PrepareResponse() override {

    //     }

    //     std::shared_ptr<void> get() override { return std::static_pointer_cast<void>(response_); }
    //     // http::response<http::string_body>& get() { return response_; }
    // };

    // class FileResponseWrapper : public ResponseInterface {
    //     std::shared_ptr<http::response<http::file_body>> response_;

    // public:
    //     FileResponseWrapper(http::response<http::file_body> response)
    //         : response_(std::make_shared<http::response<http::file_body>>(std::move(response))) {
    //     }

    //     void ProcessResponse() override {
    //     }

    //     void PrepareResponse() override {
            
    //     }

    //     std::shared_ptr<void> get() override { return std::static_pointer_cast<void>(response_); }
    //     // http::response<http::file_body>& get() { return response_; }
    // };

    class RequestHandler {
    public:
        explicit RequestHandler(model::Game& game);
        RequestHandler(model::Game& game, const char* root_dir);

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        ResponseVariant HandleRequest(StringRequest&& req);

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            ResponseVariant response = HandleRequest(std::move(req));
            if (holds_alternative<StringResponse>(response)) {
                auto& string_response = std::get<StringResponse>(response);
                send(std::move(string_response));
            } else if (holds_alternative<FileResponse>(response)) {
                auto& file_response = std::get<FileResponse>(response);
                send(std::forward<decltype(file_response)>(file_response));
            }
            
            // send(std::move(responseContent));
        }

    private:
        model::Game& game_;
        const char* root_dir_;

        template <typename Handler>
        StringResponse HandleApiRequest(const std::vector<std::string>& path_components, const Handler& json_response) {
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
        ResponseVariant HandleGetFileRequest(std::string req_path, const Handler& json_response) {
            fs::path base_path = fs::weakly_canonical(root_dir_);
            fs::path abs_path = ProcessingAbsPath(root_dir_, req_path);

            auto abs_path_str = abs_path.string();

            if (IsSubPath(abs_path, base_path)) {
                // auto result = util::ReadStaticFile(abs_path);

                // auto result = util::TestFunc(abs_path);
                // return json_response(http::status::ok, util::ReadFromFileIntoString(abs_path));
                return json_response(http::status::ok, util::TestFunc(abs_path));
                // return result;
            }

            return HandleBadRequest(json_response);
        }

        template <typename Handler>
        ResponseVariant ProcessRequest(std::string_view path, const Handler& json_response) {
            url::UrlParser parser( (std::string(path)) );
            auto path_components = parser.GetComponents();

            if (path_components.size() > 0 && path_components[0] == "api"sv) {
                return HandleApiRequest(path_components, json_response);
            }

            auto path_sv = std::string(path.data());
            return HandleGetFileRequest(path_sv, json_response);
        }

        template <typename Handler>
        StringResponse HandleGetMapsRequest(const Handler& json_response) const {
            std::string maps = json_loader::MapSerializer::SerializeMapsMainInfo(game_.GetMaps());

            return json_response(http::status::ok, maps);
        }

        template <typename Handler>
        StringResponse HandleGetMapDetailsRequest(std::string_view map_id, const Handler& json_response) const {
            const model::Map::Id id{std::string(map_id)};
            auto map_ptr = game_.FindMap(id);
            
            if (map_ptr) {
                std::string map = boost::json::serialize(json_loader::MapSerializer::SerializeSingleMap(*map_ptr));
                return json_response(http::status::ok, map);
            }

            return json_response(http::status::not_found, JsonResponseBuilder::NotFound());
        }

        template <typename Handler>
        static StringResponse HandleBadRequest(const Handler& json_response) {
            return json_response(http::status::bad_request, JsonResponseBuilder::BadRequest());
        }
    };

}  // namespace http_handler