#include "request_handler.h"

#include "boost/beast/core/string_type.hpp"
#include "router.h"
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

    /* std::string ErrorHandler::NotFound(std::string_view code, std::string_view error_message) {
        json::value jv = {
            {"code", code},
            {"message", error_message}
        };

        return json::serialize(jv);
    }

    std::string ErrorHandler::NotAllowed(std::string_view code, std::string_view error_message) {
        json::value jv = {
            {"code", code},
            {"message", error_message}
        };

        return json::serialize(jv);
    } */

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
            std::make_unique<HTTPResponseMaker>(
            [this](const StringRequest& req, JsonResponseHandler json_response) -> ResponseVariant {
                return api_handler_.JoinGame(req, json_response);
            }));

        router_->AddRoute({"GET", "HEAD"}, "/api/v1/game/players", 
            std::make_unique<HTTPResponseMaker>(
            [this](const StringRequest& req, JsonResponseHandler json_response) -> ResponseVariant {
                return api_handler_.GetPlayersRequest(req, json_response);
            }));

        router_->AddRoute({"GET"}, ":", 
            std::make_unique<HTTPResponseMaker>(
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
        std::string body = req.body();
        try {
            obj = json::parse(body).as_object();
        } catch (...) {
            return ErrorHandler::MakeBadRequestResponse(json_response, 
                                                        "invalidArgument", 
                                                        "Join game request parse error");
        }

        return std::nullopt;
    }

    std::optional<StringResponse> 
    ApiRequestHandler::IsAllowedMethod(const StringRequest& req,
                                       JsonResponseHandler json_response,
                                       std::string message) const {
        if (router_->IsAllowedMethod(req.method_string().data(), req.target().data())) {
            return std::nullopt;
        }

        std::vector<std::string> allowed_methods = 
            router_->FindPath(req.method_string().data(), req.target().data());

        if (message == "") {
            message = "Only ";
            bool flag = true;
            for (const auto& str: allowed_methods) {
                if (!flag) { message += " "; }

                flag = false;
                message += str;
            }
        }

        return ErrorHandler::MakeNotAllowedResponse(json_response, allowed_methods,
                                                    "invalidMethod", message);
    }

    StringResponse ApiRequestHandler::JoinGame(const StringRequest& req, 
                                               const JsonResponseHandler& json_response) {
        if (auto optional = IsAllowedMethod(req, json_response)) {
            return optional.value();
        }

        json::object object;  // modify by ParseJoinRequest
        if (auto optional = ParseJoinRequest(req, json_response, object)) { 
            return optional.value(); 
        }
        
        if (!object.count("mapId")) {
            return ErrorHandler::MakeNotFoundResponse(json_response, "mapNotFound",
                                                      "Map not found");
        }
        std::string map_id = object.at("mapId").as_string().c_str();
        std::string user_name = object.at("userName").as_string().c_str();
        auto session = game_.FindGameSession(model::Map::Id{map_id.data()});
        if (session == nullptr) {
            return ErrorHandler::MakeNotFoundResponse(json_response, "mapNotFound", 
                                                      "Map not found");
        }

        if (user_name.size() == 0) {
            return ErrorHandler::MakeBadRequestResponse(json_response, "invalidArgument", 
                                                        "Invalid name");
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

    bool ApiRequestHandler::IsValidAuthToken(const std::string& auth_header) const {
        // Предполагаем, что токен представлен в формате "Bearer <token>"
        const std::string prefix = "Bearer ";
        if (auth_header.find(prefix) != 0) {
            return false;
        }

        std::string token = auth_header.substr(prefix.size());
        
        // Проверка на соответствие шаблону
        const std::regex token_pattern("^[0-9a-f]{32}$");
        return std::regex_match(token, token_pattern);
    }

    StringResponse ApiRequestHandler::GetPlayersRequest(const StringRequest& req, 
                                                        JsonResponseHandler json_response) const {
        if (auto optional = IsAllowedMethod(req, json_response)) {
            return optional.value();
        }

        auto auth_header = req[http::field::authorization];
        if (auth_header.empty() || !IsValidAuthToken(auth_header.data())) {
            return ErrorHandler::MakeUnauthorizedResponse(json_response, "invalidToken", 
                                                          "Authorization header is missing");
        }

        auto player = players_.GetPlayerByToken(app::Token{auth_header.data()});
        if (player == nullptr) {
            return ErrorHandler::MakeUnauthorizedResponse(json_response, "unknownToken", 
                                                          "Player token has not been found");
        }

        auto names = player->GetListOfPlayersNickNames();
        json::object players_json;
        int index = 0;
        
        for (const auto& player_name : names) {
            players_json[std::to_string(index)] = {
                {"name", player_name}
            };

            index++;
        }

        std::string response_body = json::serialize(players_json);
        auto result = json_response(http::status::ok, response_body, ContentType::APP_JSON);
        result.set(http::field::cache_control, "no-cache");
        return result;
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
        : game_(game), root_dir_(path), players_(players), router_(router){
    }

}  // namespace http_handler