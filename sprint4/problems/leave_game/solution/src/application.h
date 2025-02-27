#pragma once

#include "model_serialization.h"
#include "sdk.h"
#include "type_declarations.h"
#include "infrastructure.h"
#include "player.h"
#include "model.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <random>
#include <algorithm>
#include <unordered_map>
#include <iostream>
#include <boost/functional/hash.hpp>
#include <boost/json.hpp>
#include <boost/beast/http.hpp>



namespace app {

    using milliseconds = std::chrono::milliseconds;

    namespace detail {
        struct TokenTag {};
    }
    
    using Token = util::Tagged<std::string, detail::TokenTag>;

    class PlayerTokens {
    public:
        PlayerTokens() = default;

        Token AddPlayer(std::shared_ptr<Player::Player> player);

        Token FindTokenByPlayer(std::shared_ptr<Player::Player> player) {
            for (const auto& [token, stored_player] : token_to_player_) {
                if (stored_player == player) {
                    return token;
                }
            }

            return {};
        } 

        std::shared_ptr<Player::Player> FindPlayerByToken(const Token& token) const;

    private:
        using TokenHasher = util::TaggedHasher<Token>;
        std::unordered_map<Token, std::shared_ptr<Player::Player>, TokenHasher> token_to_player_;

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

        std::shared_ptr<Player::Player> GetPlayerByToken(const Token& token) const;

        std::shared_ptr<Player::Player> FindByDogAndMapId(model::Dog::Id dog_id, model::Map::Id map_id);

        void Remove(model::Dog::Id dog_id, model::Map::Id map_id);

        Token FindTokenByPlayer(std::shared_ptr<Player::Player> player);

    private:
        PlayerTokens player_tokens_;
        std::unordered_map<std::pair<uint64_t, std::string>, 
            std::shared_ptr<Player::Player>, boost::hash<std::pair<uint64_t, std::string>>> players_;

    };

    class Application {
    public:
        explicit Application(model::Game& game);

        void SetApplicationListener(ApplicationListener& listener);

        const std::string GetSerializedPlayersList(const Token& token) const;
        const std::string GetSerializedGameState(const Token& token) const;

        bool HasPlayerToken(Token token) const;

        std::optional<http_handler::StringResponse> 
        MovePlayer(const Token& token,  http_handler::JsonResponseHandler json_response, 
                   std::string direction = "");

        void MovePlayer(const Token& token, std::string direction = "");

        Token AddPlayer(std::shared_ptr<model::Dog> dog, 
                        std::shared_ptr<model::GameSession> session);

        void Tick(milliseconds delta_time) const;

        serialization::GameSer SerializeGame() const { return serialization::GameSer(game_); }

        void LoadGameFromFile(model::Game game) {
            game_.SetCommonData(game.GetCommonData());
            game_.SetDefaultDogSpeed(game.GetDefaultDogSpeed());
            game_.SetDefaultTickTime(game.GetDefaultTickTime());
        }

    private:

        const std::vector<std::string> GetPlayersList(const Token& token) const;

        std::shared_ptr<Player::Player> 
        FindExistingPlayer(model::Dog::Id dog_id, model::Map::Id map_id);

        void RemovePlayerFromSession(std::shared_ptr<Player::Player> player, 
                                    std::shared_ptr<model::GameSession> session);

        Token HandleExistingPlayer(std::shared_ptr<Player::Player> player, 
                                   std::shared_ptr<model::GameSession> new_session);

        Token CreateNewPlayer(std::shared_ptr<model::Dog> dog, std::shared_ptr<model::GameSession> session);

        Token FindTokenByPlayer(std::shared_ptr<Player::Player> player);


		model::Game& game_;
		Players players_;
        ApplicationListener* listener_ = nullptr;
    };
}

namespace serialization {
    class SerializingListener {
    public:
        SerializingListener(app::Application& app, 
                            const std::string& state_file, 
                            milliseconds save_period);

        void OnTick(milliseconds delta);

        void SaveStateToFile();
        void LoadStateFromFile();

    private:
        app::Application& app_;
        std::string state_file_;
        milliseconds save_period_;
        milliseconds time_since_last_save_{0};
    };
}