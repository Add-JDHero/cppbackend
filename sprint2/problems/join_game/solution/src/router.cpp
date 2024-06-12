#include "router.h"
#include "request_handler.h"
#include "handlers.h"

namespace router {

    TrieNode::TrieNode() : params(std::make_unique<ParamsSet>()) {}

    std::string_view Trie::GetMethod() const {
        return method_;
    }

    Trie::Trie() : root_(std::make_unique<TrieNode>()) {}

    Trie::Trie(std::string method) 
        : root_(std::make_unique<TrieNode>())
        , method_(method) {
    }

    void Trie::AddRoute(const std::string& method, 
                        const std::string& path, 
                        HandlerPtr handler, 
                        bool intermediate) {
        TrieNode* node = root_.get();
        auto segments = SplitPath(path);
        for (const auto& segment : segments) {
            node = AddSegmentNode(node, segment);
        }

        if (intermediate) {
            node->intermediateHandlers.push_back(std::move(handler));
        } else {
            node->handlers.push_back(std::move(handler));
        }
    }


    TrieNode* AddParam(TrieNode* node, const std::string& segment) {
        if (!node->children.count("param")) {
            node->children["param"] = std::make_unique<TrieNode>();
            node->children["param"]->is_param = true;
            node->children["param"]->params->insert(segment.substr(1));
        }

        node = node->children["param"].get();

        return node;
    }

    TrieNode* Trie::AddSegmentNode(TrieNode* node, const std::string& segment) {
        segment.starts_with(':') ? node = AddParam(node, segment) : node = GetNode(node, segment);

        return node;
    }

    std::vector<HandlerPtr>* Trie::GetHandlers(const std::string& path/* , 
                                               std::unordered_map<std::string, std::string>& params */) {
        TrieNode* node = root_.get();
        auto segments = SplitPath(path);
        for (const auto& segment : segments) {
            node = GetNextNode(node, segment/* , params */);
            if (!node) {
                return nullptr;
            }
        }

        if (!node->handlers.empty()) {
            return &node->handlers;
        } else if (!node->intermediateHandlers.empty()) {
            return &node->intermediateHandlers;
        }
        return nullptr;
    }

    TrieNode* Trie::GetNode(TrieNode* node, const std::string& segment) {
        if (node->children.find(segment) == node->children.end()) {
            node->children[segment] = std::make_unique<TrieNode>();
        }
        return node->children[segment].get();
    }

    
    TrieNode* Trie::GetNextNode(TrieNode* node, const std::string& segment /*, 
                                std::unordered_map<std::string, std::string>& params */) {
        if (node->children.find(segment) != node->children.end()) {
            node = node->children[segment].get();
        } else if (node->children.find("param") != node->children.end()) {
            node = node->children["param"].get();
            /* for (const auto& param : node->params) {
                params[param.first] = segment;
            } */
        } else {
            return nullptr;
        }

        return node;
    }

    std::vector<std::string> Trie::SplitPath(const std::string& path) const {
        std::vector<std::string> segments;
        std::istringstream stream(path);
        std::string segment;
        while (std::getline(stream, segment, '/')) {
            if (!segment.empty()) {
                segments.push_back(segment);
            }
        }
        return segments;
    }

    Router::Router() {}

    void Router::AddRoute(const std::vector<std::string>& methods, 
                          const std::string& path, 
                          HandlerPtr handler, 
                          bool intermediate) {
        for (const auto& method : methods) {
            if (!trie_.count(method)) {
                trie_[method] = std::make_unique<Trie>();
            }
            trie_[method]->AddRoute(method, path, handler, intermediate);
        }
        
        path_to_allowed_methods_[path] = methods;
    }

    http_handler::ResponseVariant Router::Route(const http_handler::StringRequest& req) {
        auto ver = req.version();
        auto keep = req.keep_alive();
        auto json_response = 
            [this, version = ver, keep_alive = keep](http::status status, std::string body = {}, 
                                                     std::string_view content_type = 
                                                        http_handler::ContentType::APP_JSON) {
            return http_handler::HttpResponse::MakeStringResponse(status, body, version, 
                                                                  keep_alive, content_type);
        };

        // std::unordered_map<std::string, std::string> params;
        auto method = std::string(req.method_string());
        auto path = util::UrlDecode(std::string(req.target()));

        if (trie_.find(method) != trie_.end()) {
            auto handlers = trie_[method]->GetHandlers(path/* , params */);
            if (handlers) {
                for (auto& handler : *handlers) {
                    auto response = handler->Invoke(req, json_response);
                    if (std::holds_alternative<http_handler::StringResponse>(response) || 
                        std::holds_alternative<http_handler::FileResponse>(response)) {
                        return std::visit([&req](auto&& arg) -> http_handler::ResponseVariant {
                            return std::move(arg);
                        }, std::move(response));
                    }
                }
            } else {
                return http_handler::ErrorHandler::
                    MakeNotAllowedResponse(json_response, path_to_allowed_methods_[path],
                                            "invalidMethod","Invalid method");
            } 
        } else {
            return http_handler::ErrorHandler::
                MakeNotAllowedResponse(json_response, path_to_allowed_methods_[path],
                                       "invalidMethod", "Invalid method");
        }

        return http_handler::ErrorHandler::MakeBadRequestResponse(json_response);
    }

    bool Trie::HasRoute(const std::string& method, const std::string& path) {
        bool has_route = false;
        auto path_components = SplitPath(path);

        TrieNode* node = root_.get();
        for (const auto& segment: path_components) {
            node = GetNextNode(node, segment);

            if (!node) { return false; }
        }

        return true;
    }

    bool Router::HasRoute(const std::string& method, const std::string& path) {
        if (trie_.count(method)) {
            return trie_[method]->HasRoute(method, path);
        } 

        return false;
    }

    std::vector<std::string> Router::FindPath(const std::string& method, const std::string& path) {
        std::vector<std::string> relevant_methods;
        for (const auto& [method, trie]: trie_) {
            if (trie_[method]->HasRoute(method, path)) {
                relevant_methods.push_back(method);
            }
        }

        return relevant_methods;
    }

    bool Router::IsAllowedMethod(const std::string& method, const std::string& path) {
        return FindPath(method, path).size() > 0;
    }
}
