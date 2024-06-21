#include "request_handler.h"

// #include "boost/beast/core/string_type.hpp"
#include "application.h"
#include "handlers.h"
#include "router.h"
#include "util.h"
#include <cstdint>
#include <exception>
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

    RequestHandler::RequestHandler(model::Game& game, Strand& api_strand, 
                                   fs::path path, app::Application& app)
        : game_{game}
        , api_strand_(api_strand)
        , root_dir_(path)
        , app_(app) {
    }



    StringResponse ApiRequestHandler::RouteRequest(const StringRequest& req) {
        return router_->Route(req);
    }

    void ApiRequestHandler::SetupEndPoits() {
        using JsonResponseHandler = 
            std::function<StringResponse(http::status, std::string, std::string_view)>;

        using TokenHandler = 
            std::function<std::optional<StringResponse>(const StringRequest&, const JsonResponseHandler&)>;

        auto token_handler = std::make_shared<TokenHandler>(
            [this](const StringRequest& req, const JsonResponseHandler& json_response) 
            -> std::optional<StringResponse> {
                if (auto optional = this->TokenHandler(req, json_response)) {
                    return optional;
                }

                return std::nullopt;
            }
        );


        router_->AddRoute({"GET", "HEAD"}, "/api/v1/maps", 
            std::make_unique<HTTPResponseMaker>(
            [this](const StringRequest& req, const JsonResponseHandler& json_response) -> StringResponse {
                url::UrlParser parser(std::string(req.target()));
                auto path_components = parser.GetComponents();
                return this->GetMapsRequest(json_response);
            }
        ));

        router_->AddRoute({"GET", "HEAD"}, "/api/v1/maps/:id", 
            std::make_unique<HTTPResponseMaker>(
            [this](const StringRequest& req, const JsonResponseHandler& json_response) -> StringResponse {
                url::UrlParser parser(std::string(req.target()));
                auto map_id = parser.GetLastComponent();
                return this->GetMapDetailsRequest(json_response, map_id);
            }
        ));

        router_->AddRoute({"POST"}, "/api/v1/game/join", 
            std::make_shared<HTTPResponseMaker>(
            [this](const StringRequest& req, const JsonResponseHandler& json_response) -> StringResponse {
                return this->JoinGame(req, json_response);
            }));

        router_->AddRoute({"GET", "HEAD"}, "/api/v1/game/players", 
            std::make_shared<HTTPResponseMaker>(
            [this ](const StringRequest& req, const JsonResponseHandler& json_response) -> StringResponse {
                /* return this->ExecuteAuthorized(GetPlayersRequest, 
                                               req, json_response); */
                
                return this->GetPlayersRequest(req, json_response);
            }));

        router_->AddRoute({"GET", "HEAD"}, "/api/v1/game/state", 
            std::make_shared<HTTPResponseMaker>(
            [this](const StringRequest& req, const JsonResponseHandler& json_response) -> StringResponse {
                // return this->ExecuteAuthorized(this->GetGameState(req, json_response), req, json_response);

                return this->GetGameState(req, json_response);
            }));
            
        router_->AddRoute({"POST"}, "/api/v1/game/player/action", 
            std::make_shared<HTTPResponseMaker>(
            [this](const StringRequest& req, const JsonResponseHandler& json_response) -> StringResponse {
                /* return this->ExecuteAuthorized(this->MoveUnit(req, json_response), 
                                               req, json_response); */

                return this->MoveUnit(req, json_response);
            }));

        router_->AddRoute({"POST"}, "/api/v1/game/tick", 
            std::make_shared<HTTPResponseMaker>(
            [this](const StringRequest& req, const JsonResponseHandler& json_response) -> StringResponse {
                return this->TickRequest(req, json_response);
            }));
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
                                                                      const JsonResponseHandler& json_response,
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

        std::shared_ptr<model::Dog> dog = std::make_shared<model::Dog>(user_name);

        app::Token token = app_.AddPlayer(dog, session);

        json::value value = {
            { SpecialStrings::AUTH_TOKEN.data(), *token },
            { SpecialStrings::PLAYER_ID.data(),  dog->GetId() }
        };

        std::string val = json::serialize(value);
        
        return json_response(http::status::ok, std::move(val), ContentType::APP_JSON.data());
    }

    bool ApiRequestHandler::IsValidAuthToken(std::string& token) const {
        const std::regex token_pattern("^[0-9a-fA-F]{32}$");
        return std::regex_match(token, token_pattern);
    }

    std::optional<StringResponse> 
    ApiRequestHandler::TokenHandler(const StringRequest& req, const JsonResponseHandler& json_response) const {
        std::string token = "";
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

    std::optional<StringResponse> 
    ApiRequestHandler::TokenHandler(const StringRequest& req, 
                                    const JsonResponseHandler& json_response,
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

    std::optional<StringResponse> 
    ApiRequestHandler::ParseContentType(const StringRequest& req,
                                        const JsonResponseHandler& json_response) const {
        try {
            std::string content_header = std::string(req.base().at(http::field::content_type));
            if (content_header != "application/json") { throw; } 
        } catch (...) {
            return ErrorHandler::MakeBadRequestResponse(json_response, "invalidArgument", 
                                                        "Invalid content type");
        }

        return std::nullopt;

    }

    StringResponse ApiRequestHandler::GetPlayersRequest(const StringRequest& req, 
                                                        const JsonResponseHandler& json_response) const {
        
        /* // проверка того, что токен есть и он валидный, выполняется с в другом месте 
        std::string token = util::ExtractToken(std::string(req.base().at(http::field::authorization))); */
        std::string token;
        if (auto optional = TokenHandler(req, json_response, token)) {
            return optional.value();
        }
        
        if (!app_.HasPlayerToken(app::Token{token})) {
            return ErrorHandler::MakeUnauthorizedResponse(json_response, "unknownToken", 
                                                          "Player token has not been found");
        }

        std::string response_body = app_.GetSerializedPlayersList(app::Token{token});
        return json_response(http::status::ok, response_body, ContentType::APP_JSON);
    }

    std::optional<StringResponse> 
    ApiRequestHandler::ParseMoveJson(const JsonResponseHandler& json_response, 
                                     std::string data,
                                     std::string& direction) const {
        std::string dir;
        try {
            json::value move_request = json::parse(data);
            dir = move_request.at("move").as_string().data();
        } catch (...) {
            return ErrorHandler::MakeBadRequestResponse(json_response,
                                                        "invalidArgument",
                                                        "Failed to parse action");
        }

        if (dir == "") {
            direction = "";
        } else if (dir == "L") {
            direction = "L";
        } else if (dir == "R") {
            direction = "R";
        } else if (dir == "U") {
            direction = "U";
        } else if (dir == "D") {
            direction = "D";
        } else {
            return ErrorHandler::MakeBadRequestResponse(json_response,
                                                        "invalidArgument",
                                                        "Failed to parse action");
        }

        return std::nullopt;
    }

    StringResponse ApiRequestHandler::MoveUnit(const StringRequest& req, 
                                              const JsonResponseHandler& json_response) {
        /* // проверка того, что токен есть и он валидный, выполняется с в другом месте 
        std::string token = util::ExtractToken(std::string(req.base().at(http::field::authorization))); */
        std::string token;
        if (auto optional = TokenHandler(req, json_response, token)) {
            return optional.value();
        }

        if (!app_.HasPlayerToken(app::Token{token})) {
            return ErrorHandler::MakeUnauthorizedResponse(json_response, "unknownToken", 
                                                          "Player token has not been found");
        }

        if (auto optional_parse_error = ParseContentType(req, json_response)) {
            return optional_parse_error.value();
        }
        
        std::string direction = "";
        if (auto optional_parse_error = ParseMoveJson(json_response, req.body().data(), direction)) {
            return optional_parse_error.value();
        } // modify direction value if Move Json was valid!

        app_.MovePlayer(app::Token{token}, direction);

        std::string response_body = "{}";
        return json_response(http::status::ok, response_body, ContentType::APP_JSON);
    }

    StringResponse ApiRequestHandler::GetGameState(const StringRequest& req,
                                    const JsonResponseHandler& json_response) const {
        std::string token;
        if (auto optional = TokenHandler(req, json_response, token)) {
            return optional.value();
        }

        if (!app_.HasPlayerToken(app::Token{token})) {
            return ErrorHandler::MakeUnauthorizedResponse(json_response, "unknownToken", 
                                                          "Player token has not been found");
        }

        auto response_body = app_.GetSerializedGameState(app::Token{token});
        return json_response(http::status::ok, std::move(response_body), ContentType::APP_JSON);
    }

    bool IsDigit(char c) {
        return c >= '0' && c <= '9';
    }

    bool IsUnsignedNumber(std::string_view value) {
        for (const auto c: value) {
            if (!IsDigit(c)) { return false; }
        }

        return true;
    }

    std::optional<StringResponse> 
    ApiRequestHandler::ParseTickJson(const JsonResponseHandler& json_response, 
                                     std::string data,
                                     uint64_t& milliseconds) const {
        uint64_t value;
        try {

            boost::json::value jv = boost::json::parse(data);
            value = json::value_to<uint64_t>(jv.at("timeDelta"));
            if (value == 0) { 
                jv.as_string();
            }
        } catch (...) {
            return ErrorHandler::MakeBadRequestResponse(json_response,
                                                        "invalidArgument",
                                                        "Failed to parse tick request JSON");
        }

        milliseconds = value;

        return std::nullopt;
    }

    StringResponse ApiRequestHandler::TickRequest(const StringRequest& req, 
                                                  const JsonResponseHandler& json_response) const {
        if (auto optional = ParseContentType(req, json_response)) {
            return optional.value();
        }

        uint64_t milliseconds;
        if (auto optional = ParseTickJson(json_response, req.body().data(), milliseconds)) {
            return optional.value();
        }

        app_.Tick(milliseconds);

        std::string response_body = "{}";
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

    ApiRequestHandler::ApiRequestHandler(model::Game& game, fs::path path, app::Application& app) 
        : game_(game), root_dir_(path), app_(app)
        , router_(std::make_unique<router::Router>()) {
            SetupEndPoits();
    }

}  // namespace http_handler