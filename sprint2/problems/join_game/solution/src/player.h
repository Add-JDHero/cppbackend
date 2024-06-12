#pragma once

#include "sdk.h"
#include "model.h"

#include <cstdint>
#include <memory>
#include <random>
#include <unordered_map>
#include <iostream>
#include <boost/functional/hash.hpp>

namespace detail {
    struct TokenTag {};
}  

namespace app {
    using Token = util::Tagged<std::string, detail::TokenTag>;

	class Player {
	public:
        Player() = delete;
        Player(std::shared_ptr<model::Dog> dog, std::shared_ptr<model::GameSession> game_session):
            dog_(dog), 
            game_session_(game_session) {
            game_session->AddDog(dog_);
        }

        model::Dog::Id GetDogId() {
            return dog_->GetId();
        }

        const std::shared_ptr<model::GameSession> GetGameSession() const {
            return game_session_;
        }

	private:
        std::shared_ptr<model::Dog> dog_;
		std::shared_ptr<model::GameSession> game_session_;
		
	};


    class PlayerTokens {
    public:
        PlayerTokens() = default;

        Token AddPlayer(std::shared_ptr<Player> player) {
            Token token = GenerateToken();
            token_to_player_[token] = player;

            return token;
        }

        std::shared_ptr<Player> FindPlayerByToken(const Token& token) {
            auto player = token_to_player_.find(token);
            if (player != token_to_player_.end())
                return player->second;

            return nullptr;
        }

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
        Token Add(std::shared_ptr<model::Dog> dog, std::shared_ptr<model::GameSession> game_session) {
            std::shared_ptr<Player> player = std::make_shared<Player>(dog, game_session);
            Token token = player_tokens_.AddPlayer(player);
            players_[{dog->GetId(), *(game_session->GetMapId())}] = 
                std::make_shared<Player>(dog, game_session);
        
            return token;
        }

        /* uint64_t GetPlayerID(model::Dog::Id dog_id, model::Map::Id map_id) {
            auto player = FindByDogAndMapId(dog_id, map_id);

            auto id = player->GetDogId();

            return id;
        } */

        std::shared_ptr<Player> GetPlayerByToken(Token token) {
            return player_tokens_.FindPlayerByToken(token);
        }

        std::shared_ptr<Player> FindByDogAndMapId(model::Dog::Id dog_id, model::Map::Id map_id) {
            return players_[{dog_id, *map_id}];
        }

    private:
        PlayerTokens player_tokens_;
        std::unordered_map<std::pair<uint64_t, std::string>, 
            std::shared_ptr<Player>, boost::hash<std::pair<uint64_t, std::string>>> players_;

    };

    class Application {
    public:
        explicit Application(model::Game& game, Players& players);
        
        const std::vector<std::string> GetPlayersList(Token token) const;

    private:
		model::Game& game_;
		Players& players_;

    };
}