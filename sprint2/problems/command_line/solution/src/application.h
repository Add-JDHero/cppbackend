#pragma once

#include "sdk.h"
// #include "request_handler.h"
#include "type_declarations.h"
#include "model.h"

#include <cstdint>
#include <memory>
#include <random>
#include <unordered_map>
#include <iostream>
#include <boost/functional/hash.hpp>
#include <boost/json.hpp>
#include <boost/beast/http.hpp>

namespace detail {
    struct TokenTag {};
}  

namespace app {
    using Token = util::Tagged<std::string, detail::TokenTag>;

	class Player {
	public:
        Player() = delete;
        Player(std::shared_ptr<model::Dog> dog, std::shared_ptr<model::GameSession> game_session);
        model::Dog::Id GetDogId();

        void MovePlayer(std::string direction = "");

        const std::shared_ptr<model::GameSession> GetGameSession() const;

	private:
        std::shared_ptr<model::Dog> dog_;
		std::shared_ptr<model::GameSession> game_session_;
		
	};


    class PlayerTokens {
    public:
        PlayerTokens() = default;

        Token AddPlayer(std::shared_ptr<Player> player);

        std::shared_ptr<Player> FindPlayerByToken(const Token& token) const;

    private:
        using TokenHasher = util::TaggedHasher<Token>;
        std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher> token_to_player_;

        std::random_device random_device_;
        std::mt19937_64 generator1_{[this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }()};
        std::mt19937_64 generator2_{[this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }()};

        Token GenerateToken();
    };

    class Players {
    public:
        Token Add(std::shared_ptr<model::Dog> dog, std::shared_ptr<model::GameSession> game_session);

        std::shared_ptr<Player> GetPlayerByToken(Token token) const;

        std::shared_ptr<Player> FindByDogAndMapId(model::Dog::Id dog_id, model::Map::Id map_id);

    private:
        PlayerTokens player_tokens_;
        std::unordered_map<std::pair<uint64_t, std::string>, 
            std::shared_ptr<Player>, boost::hash<std::pair<uint64_t, std::string>>> players_;

    };

    class Application {
    public:
        explicit Application(model::Game& game);
        
        const std::string GetSerializedPlayersList(Token token) const;
        const std::string GetSerializedGameState(Token token) const;

        bool HasPlayerToken(Token token) const;

        std::optional<http_handler::StringResponse> 
        MovePlayer(Token token,  http_handler::JsonResponseHandler json_response, 
                   std::string direction = "");

        void MovePlayer(Token token, std::string direction = "");

        Token AddPlayer(std::shared_ptr<model::Dog> dog, 
                        std::shared_ptr<model::GameSession> session);

        void Tick(double delta_time) const;

    private:

        const std::vector<std::string> GetPlayersList(Token token) const;

		model::Game& game_;
		Players players_;

    };
}