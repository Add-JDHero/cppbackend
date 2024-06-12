#pragma once

#include "sdk.h"
#include "type_declarations.h"
#include "handlers.h"

#include <boost/beast/http.hpp>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <variant>
#include <sstream>
#include <memory>

namespace router {

    using namespace http_handler;

    using HandlerPtr = std::shared_ptr<HandlerBase>;
    using ParamsSet = std::unordered_set<std::string>;

    class TrieNode {
    public:
        TrieNode();
        
        std::unordered_map<std::string, std::unique_ptr<TrieNode>> children;
        std::unique_ptr<ParamsSet> params;
        std::vector<HandlerPtr> handlers;
        std::vector<HandlerPtr> intermediateHandlers;
        HandlerPtr default_handler;
        bool is_param = false;

    };

    class Trie {
    public:
        Trie();

        explicit Trie(std::string method);

        void AddRoute(const std::string& method, 
                      const std::string& path, 
                      HandlerPtr handler, 
                      bool intermediate = false);
        
        std::vector<HandlerPtr>* GetHandlers(const std::string& path/* , 
                                             std::unordered_map<std::string, std::string>& params */);
                                             
        std::vector<std::string> SplitPath(const std::string& path) const;

        bool HasRoute(const std::string& method, const std::string& path);

    private:
        std::unique_ptr<TrieNode> root_;
        std::string method_;


        std::string_view GetMethod() const;

        TrieNode* GetNode(TrieNode* node, const std::string& segment);
        TrieNode* GetNextNode(TrieNode* node, 
                              const std::string& segment /*, 
                              std::unordered_map<std::string, std::string>& params */);
        TrieNode* AddSegmentNode(TrieNode* node, const std::string& segment);

    };

    class Router {
    public:
        Router();

        void AddRoute(const std::vector<std::string>& methods, 
                      const std::string& path, 
                      HandlerPtr handler, 
                      bool intermediate = false);

        ResponseVariant Route(const StringRequest& req);

        std::vector<std::string> FindPath(const std::string& method, const std::string& path);

        bool HasRoute(const std::string& method, const std::string& path);
        bool IsAllowedMethod(const std::string& method, const std::string& path);


    private:
        std::unordered_map<std::string, std::unique_ptr<Trie>> trie_;
        std::unordered_map<std::string, 
            std::vector<std::string>> path_to_allowed_methods_;
    };

}
