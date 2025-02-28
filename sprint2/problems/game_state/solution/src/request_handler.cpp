#include "request_handler.h"

// #include "boost/beast/core/string_type.hpp"
#include "router.h"
#include <memory>
#include <system_error>
#include <variant>
#include <regex>
#include <optional>

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
        if (rel_path.empty() || 
            (!rel_path.empty() && rel_path.string().back() == separating_chars::SLASH)) {
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

    std::string ErrorHandler::SerializeErrorResponseBody(std::string_view code, 
                                                         std::string_view error_message) {
        json::value jv = {
            {"code", code},
            {"message", error_message}
        };
        return json::serialize(jv);
    }

    StringResponse ErrorHandler::MakeBadRequestResponse(const JsonResponseHandler& json_response, 
                                                     std::string_view error_code,
                                                     std::string_view error_msg) {
        std::string response_body = 
            error_code.empty() 
                ? SerializeErrorResponseBody("badRequest", "Bad Request") 
                : SerializeErrorResponseBody(error_code, error_msg);
        return json_response(http::status::bad_request, response_body, ContentType::APP_JSON);
    }

    StringResponse ErrorHandler::MakeNotFoundResponse(const JsonResponseHandler& json_response, 
                                                   std::string_view error_code,
                                                   std::string_view error_msg) {
        std::string response_body = error_code.empty() 
            ? SerializeErrorResponseBody("notFound", "Not Found") 
            : SerializeErrorResponseBody(error_code, error_msg);
        return json_response(http::status::not_found, response_body, ContentType::APP_JSON);
    }

    StringResponse ErrorHandler::MakeNotAllowedResponse(const JsonResponseHandler& json_response,
                                                     std::vector<std::string> allowed_methods,
                                                     std::string_view error_code,
                                                     std::string_view error_msg) {
        std::string response_body = error_code.empty() 
            ? SerializeErrorResponseBody("invalidMethod", "Invalid method") 
            : SerializeErrorResponseBody(error_code, error_msg);

        std::string allowed_methods_str;
        bool flag = true;
        for (const auto& method : allowed_methods) {
            if (!flag) { allowed_methods_str += ", "; }
            
            flag = false;
            allowed_methods_str += method;
        }

        StringResponse result = json_response(http::status::method_not_allowed,response_body, 
                                              ContentType::APP_JSON);
        result.set(http::field::allow, allowed_methods_str);
        
        return result;
    }

    StringResponse ErrorHandler::MakeUnauthorizedResponse(const JsonResponseHandler& json_response,
                                                          std::string_view error_code,
                                                          std::string_view error_msg) {
        std::string response_body = error_code.empty() 
            ? SerializeErrorResponseBody("invalidToken", "Invalid token") 
            : SerializeErrorResponseBody(error_code, error_msg);

        return json_response(http::status::unauthorized, response_body, ContentType::APP_JSON);
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
        MakeResponse(response, body_sv, keep_alive, content_type);
        return response;
    }

    RequestHandler::RequestHandler(model::Game& game, Strand api_strand, fs::path path)
        : game_{game}
        , router_(std::make_shared<router::Router>())
        , api_strand_(api_strand)
        , root_dir_(path) {
            SetupEndPoits();
    }

    void RequestHandler::SetupEndPoits() {
        using JsonResponseHandler = 
            std::function<StringResponse(http::status, std::string, std::string_view)>;

        router_->AddRoute({"GET", "HEAD"}, "/api/v1/maps", 
            std::make_unique<HTTPResponseMaker>(
            [this](const StringRequest& req, JsonResponseHandler json_response) -> ResponseVariant {
                url::UrlParser parser(std::string(req.target()));
                auto path_components = parser.GetComponents();
                return this->api_handler_.GetMapsRequest(json_response);
            }
        ));

        router_->AddRoute({"GET", "HEAD"}, "/api/v1/maps/:id", 
            std::make_unique<HTTPResponseMaker>(
            [this](const StringRequest& req, JsonResponseHandler json_response) -> ResponseVariant {
                url::UrlParser parser(std::string(req.target()));
                auto map_id = parser.GetLastComponent();
                return this->api_handler_.GetMapDetailsRequest(json_response, map_id);
            }
        ));

        router_->AddRoute({"POST"}, "/api/v1/game/join", 
            std::make_shared<HTTPResponseMaker>(
            [this](const StringRequest& req, JsonResponseHandler json_response) -> ResponseVariant {
                return api_handler_.JoinGame(req, json_response);
            }));

        router_->AddRoute({"GET", "HEAD"}, "/api/v1/game/players", 
            std::make_shared<HTTPResponseMaker>(
            [this](const StringRequest& req, JsonResponseHandler json_response) -> ResponseVariant {
                return api_handler_.GetPlayersRequest(req, json_response);
            }));

        router_->AddRoute({"GET", "HEAD"}, "/api/v1/game/state", 
            std::make_shared<HTTPResponseMaker>(
            [this](const StringRequest& req, JsonResponseHandler json_response) -> ResponseVariant {
                return api_handler_.GetGameState(req, json_response);
            }));

        router_->AddRoute({"GET"}, ":", 
            std::make_shared<HTTPResponseMaker>(
            [this](const StringRequest& req, JsonResponseHandler json_response) -> ResponseVariant {
                this->file_handler_.HandleRequest(req, json_response);

                return {}; 
            }
        ));
    }

    StringResponse ApiRequestHandler::GetMapsRequest(const JsonResponseHandler& json_response) const {
        std::string maps = json_loader::MapSerializer::SerializeMapsMainInfo(game_.GetMaps());

        return json_response(http::status::ok, maps, ContentType::APP_JSON);
    }

    StringResponse ApiRequestHandler::GetMapDetailsRequest(const JsonResponseHandler& json_response,
                                                           std::string_view map_id) const {
        const model::Map::Id id{std::string(map_id)};
        auto map_ptr = game_.FindMap(id);
        
        if (map_ptr) {
            auto map_json = json_loader::MapSerializer::SerializeSingleMap(*map_ptr);
            std::string serialized_map = boost::json::serialize(std::move(map_json));
            return json_response(http::status::ok, serialized_map, ContentType::APP_JSON);
        }

        return ErrorHandler::MakeNotFoundResponse(json_response, "mapNotFound", "Map not found");
    }

    std::optional<StringResponse> ApiRequestHandler::ParseJoinRequest(const StringRequest& req, 
                                                                      JsonResponseHandler json_response,
                                                                      json::object& obj) const {
        std::string body = req.body().c_str();

        json::error_code ec;
        auto value = json::parse(body, ec);
    
        if (ec) {
            std::cout << ec.what() << std::endl;
            return ErrorHandler::MakeBadRequestResponse(json_response, 
                                                        "invalidArgument", 
                                                        "Join game request parse error");
        }
        obj = value.as_object();

        return std::nullopt;
    }

    StringResponse ApiRequestHandler::JoinGame(const StringRequest& req, 
                                               const JsonResponseHandler& json_response) {
        json::object object;  // modify by ParseJoinRequest
        if (auto optional = ParseJoinRequest(req, json_response, object)) { 
            return optional.value(); 
        }
        
        if (!object.count("userName")) {
            return ErrorHandler::MakeBadRequestResponse(json_response, "invalidArgument",
                                                       "Invalid name");
        }
        if (!object.count("mapId")) {
            return ErrorHandler::MakeBadRequestResponse(json_response, "invalidArgument",
                                                       "Invalid mapId");
        }
        
        std::string map_id = object.at("mapId").as_string().c_str();
        std::string user_name = object.at("userName").as_string().c_str();
        auto session = game_.FindGameSession(model::Map::Id{map_id.data()});
        if (user_name.size() == 0) {
            return ErrorHandler::MakeBadRequestResponse(json_response, "invalidArgument", 
                                                        "Invalid name");
        }
        if (session == nullptr) {
            return ErrorHandler::MakeNotFoundResponse(json_response, "mapNotFound", "Map not found");
        }

        auto dog = std::make_shared<model::Dog>(user_name);
        app::Token token = players_.Add(dog, session);

        json::value value = {
            { SpecialStrings::AUTH_TOKEN.data(), *token },
            { SpecialStrings::PLAYER_ID.data(),  dog->GetId() }
        };

        std::string val = json::serialize(value);
        auto result = json_response(http::status::ok, val, ContentType::APP_JSON.data());
        result.set(http::field::cache_control, "no-cache");
        
        return result;
    }

    bool ApiRequestHandler::IsValidAuthToken(std::string& token) const {
        const std::regex token_pattern("^[0-9a-fA-F]{32}$");
        return std::regex_match(token, token_pattern);
    }

    std::string ExtractToken(std::string_view auth_header) {
        std::string_view prefix = "Bearer "sv;
        if (auth_header.starts_with(prefix)) {
            auth_header.remove_prefix(prefix.size());
        }

        return std::string(auth_header);
    }

    std::optional<StringResponse> ApiRequestHandler::TokenHandler(const StringRequest& req,
                                                                  JsonResponseHandler json_response,
                                                                  std::string& token) const {
        try {
            std::string auth_header = std::string(req.base().at(http::field::authorization));
            token = util::ExtractToken(auth_header);
        } catch (...) {
            return ErrorHandler::MakeUnauthorizedResponse(json_response, "invalidToken", 
                                                          "Authorization header is missing");
        }

        if (token.empty() || !IsValidAuthToken(token)) {
            return ErrorHandler::MakeUnauthorizedResponse(json_response, "invalidToken", 
                                                          "Authorization header is missing");
        }

        return std::nullopt;

    }

    StringResponse ApiRequestHandler::GetPlayersRequest(const StringRequest& req, 
                                                        JsonResponseHandler json_response) const {
        std::string token;
        if (auto optional = TokenHandler(req, json_response, token)) {
            return optional.value();
        }

        auto player = players_.GetPlayerByToken(app::Token{token});
        if (player == nullptr) {
            return ErrorHandler::MakeUnauthorizedResponse(json_response, "unknownToken", 
                                                          "Player token has not been found");
        }

        std::string response_body = app_.GetSerializedPlayersList(app::Token{token});
        return json_response(http::status::ok, response_body, ContentType::APP_JSON);
    }

    StringResponse ApiRequestHandler::GetGameState(const StringRequest& req,
                                    JsonResponseHandler json_response) const {
        std::string token;
        if (auto optional = TokenHandler(req, json_response, token)) {
            return optional.value();
        }

        auto player = players_.GetPlayerByToken(app::Token{token});
        if (player == nullptr) {
            return ErrorHandler::MakeUnauthorizedResponse(json_response, "unknownToken", 
                                                          "Player token has not been found");
        }

        auto response_body = app_.GetSerializedGameState(app::Token{token});
        return json_response(http::status::ok, response_body, ContentType::APP_JSON);
    }

    EmptyResponse RequestHandler::CopyResponseWithoutBody(const ResponseVariant& response) const {
        EmptyResponse new_response;

        std::visit([&new_response](auto&& arg) {
            new_response.result(arg.result());
            new_response.version(arg.version());

            // Извлечение заголовка Content-Type
            auto content_type_iter = arg.base().find(http::field::content_type);
            if (content_type_iter != arg.base().end()) {
                new_response.set(http::field::content_type, content_type_iter->value());
            }

            // Копирование всех остальных заголовков
            for (auto& header : arg.base()) {
                new_response.set(header.name_string(), header.value());
            }
        }, response);

        return new_response;
    }

    ResponseVariant FileRequestHandler::HandleRequest(const StringRequest& request, 
                                                      const JsonResponseHandler& json_response) {
        fs::path base_path = fs::weakly_canonical(root_dir_);
        std::string decoded_req_path = util::UrlDecode(std::string(request.target()));
        fs::path abs_path = ProcessingAbsPath(root_dir_.c_str(), decoded_req_path);

        if (IsSubPath(abs_path, base_path)) {
            if (fs::exists(abs_path)) {
                return util::ReadStaticFile(abs_path);
            }

            return json_response(http::status::not_found, 
                                 ErrorHandler::SerializeErrorResponseBody("fileNotFound", "File not found"),
                                 ContentType::TEXT_PLAIN);
        }

        return ErrorHandler::MakeBadRequestResponse(json_response);
    }

    FileRequestHandler::FileRequestHandler(model::Game& game, fs::path path) 
        : game_(game), root_dir_(path) {
    }

    ApiRequestHandler::ApiRequestHandler(model::Game& game, fs::path path, app::Players& players, 
                                         std::shared_ptr<router::Router> router) 
        : game_(game), root_dir_(path)
        , players_(players), router_(router){
    }

}  // namespace http_handler