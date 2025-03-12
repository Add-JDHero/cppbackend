#pragma once

#include "model_serialization.h"
#include "sdk.h"
#include "type_declarations.h"
#include "infrastructure.h"
#include "player.h"
#include "model.h"
#include "database_manager.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <algorithm>
#include <unordered_map>
#include <iostream>
#include <boost/functional/hash.hpp>
#include <boost/json.hpp>
#include <boost/beast/http.hpp>

namespace serialization {
    class SerializingListener;
}

namespace app_serialization {
    class PlayerSer;

    class PlayerTokensSer;

    class PlayersSer;

    class GameSave;
}

namespace app {

    using milliseconds = std::chrono::milliseconds;

    namespace detail {
        struct TokenTag {};
    }
    
    using Token = util::Tagged<std::string, detail::TokenTag>;

    class PlayerTokens {
        using TokenHasher = util::TaggedHasher<Token>;
    public:
        PlayerTokens() = default;

        Token AddPlayer(std::shared_ptr<player::Player> player);

        std::mt19937_64 GetFirstGenerator() { return generator1_; }
        std::mt19937_64 GetSecondGenerator() { return generator2_; }

        void RemovePlayer(Token player_token);

        Token FindTokenByPlayer(std::shared_ptr<player::Player> player) const;

        std::unordered_map<Token, std::shared_ptr<player::Player>, TokenHasher> GetTokenMap() const {
            return token_to_player_;
        }

        void RestoreTokens(const std::unordered_map<Token, std::shared_ptr<player::Player>, TokenHasher>& restored_tokens) {
            token_to_player_ = restored_tokens;
        }

        std::shared_ptr<player::Player> FindPlayerByToken(const Token& token) const;

        void RestoreTokens(PlayerTokens&& other) {
            token_to_player_ = std::move(other.token_to_player_);
        }

    private:
        std::unordered_map<Token, std::shared_ptr<player::Player>, TokenHasher> token_to_player_;

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
        Players() = default;
        using PlayerNameToPlayerId = std::unordered_map<std::string_view, int>;

        Token Add(std::shared_ptr<model::Dog> dog, std::shared_ptr<model::GameSession> game_session);

        // Возвращает всех игроков как пары (DogId -> Player)
        std::vector<std::pair<model::Dog::Id, std::shared_ptr<player::Player>>> GetPlayers() const {
            std::vector<std::pair<model::Dog::Id, std::shared_ptr<player::Player>>> result;
            for (const auto& [key, player] : players_) {
                result.emplace_back(player->GetDogId(), player);
            }
            return result;
        }

        std::shared_ptr<player::Player> FindPlayerByDogId(model::Dog::Id dog_id) const {
            for (const auto& [id, player] : GetPlayers()) {
                if (id == dog_id) {
                    return player;
                }
            }
            return nullptr;  // Если игрок не найден
        }


        std::shared_ptr<player::Player> GetPlayerByToken(const Token& token) const;

        void Remove(model::Dog::Id dog_id, model::Map::Id map_id);

        PlayerNameToPlayerId GetPlayerNamesToId() const;

        std::optional<Token> FindPlayerByName(const std::string& name) const;

        Token FindTokenByPlayer(std::shared_ptr<player::Player> player);

        void RestorePlayers(Players&& other) {
            player_tokens_.RestoreTokens(std::move(other.player_tokens_));
            players_ = std::move(other.players_);
        }

    private:
        using IdToMapId = std::pair<uint64_t, std::string>;
        PlayerTokens player_tokens_;
        std::unordered_map<IdToMapId, 
                           std::shared_ptr<player::Player>, 
                           boost::hash<std::pair<uint64_t, std::string>>> players_;

    };

    class Application {
    public:
        struct PlayerMovementInfo {
            std::string_view dog_name;
            model::Pos pos;
            model::Speed speed;
        };

        struct PlayerInfo {
            Token token;
            model::Dog::Id id;
        };

        explicit Application(model::Game& game, 
                             db::ConnectionPool& pool);

        void SetApplicationListener(ApplicationListener& listener);

        const std::string GetSerializedPlayersList(const Token& token) const;
        const std::string GetSerializedGameState(const Token& token) const;

        std::string GetGameRecords(int from = 0, int to = 10) const;

        model::MapService& GetGameMapService() {
            return game_.GetMapService();
        }

        boost::json::array GetMapLootTypes(model::Map::Id map_id) {
            return *game_.GetLootService().GetLootTypes().at(map_id);
        }

        const model::SessionService& GetSessionService() {
            return game_.GetSessionService();
        }

        const std::shared_ptr<model::GameSession> FindGameSession(model::Map::Id map_id) {
            return game_.GetSessionService().FindGameSession(map_id);
        }
        
        bool HasPlayerToken(Token token) const;

        std::optional<http_handler::StringResponse> 
        MovePlayer(const Token& token,  http_handler::JsonResponseHandler json_response, 
                   std::string direction = "");

        void MovePlayer(const Token& token, std::string direction = "");

        PlayerInfo AddPlayer(const std::string& player_name, 
                             std::shared_ptr<model::GameSession> session);

        void Tick(milliseconds delta_time);

        serialization::GameSer SerializeGame() const { 
            return serialization::GameSer(game_); 
        }

        std::unique_ptr<app_serialization::PlayersSer> SerializePlayers() const;

        void LoadGameFromFile(model::Game game);
        void LoadPlayersFromFile(Players&& players);

        void LoadGameFromFilie();


    private:
        bool IsMoving(Application::PlayerMovementInfo& old, 
                      Application::PlayerMovementInfo& current) const;

        bool IsAFK(const Application::PlayerMovementInfo& old, 
                   const Application::PlayerMovementInfo& current) const;

        void RemoveAFKPlayer(std::shared_ptr<player::Player> player);

        void RemoveAFKPlayers(double delta_time,
                              std::vector<PlayerMovementInfo>& old,
                              std::vector<PlayerMovementInfo>& current);

        void SavePlayerStatsToDB(std::shared_ptr<player::Player> dog);

        std::vector<PlayerMovementInfo> PlayersInfoSnapstot() const;

        PlayerInfo GetPlayerInfo(const std::string& name);

        bool HasPlayerByName(const std::string& name) const;

        const std::vector<std::string> GetPlayersList(const Token& token) const;

        PlayerInfo ChangePlayerSession(std::shared_ptr<player::Player>, 
                                       std::shared_ptr<model::GameSession> session);

        void RemovePlayerFromSession(std::shared_ptr<player::Player> player);

        PlayerInfo CreateNewPlayer(const std::string& player_name, 
                                   std::shared_ptr<model::GameSession> session);

        Token FindTokenByPlayer(std::shared_ptr<player::Player> player);

        db::ConnectionPool& connection_pool_;

        double dog_retirement_time_ = 15000;

		model::Game& game_;
		Players players_;
        ApplicationListener* listener_ = nullptr;
    };
}

namespace serialization {
    class SerializingListener : public ApplicationListener {
    public:
        SerializingListener(app::Application& app, 
                            const std::string& state_file, 
                            milliseconds save_period);

        void OnTick(milliseconds delta) override;

        void LoadGameDataFromFile(app_serialization::GameSave&& saved_game);

    private:
        void SaveStateToFile();
        void LoadStateFromFile() override;


        app::Application& app_;
        std::string state_file_;
        milliseconds save_period_;
        milliseconds time_since_last_save_{0};
    };
}

namespace app_serialization {
    class PlayerSer {
    public:
        PlayerSer() = default;
        
        PlayerSer(const player::Player& player) 
            : dog_(serialization::DogSer(*player.GetDog()))
            , game_session_id_(player.GetGameSession()->GetSessionId()) { // Сохраняем только ID сессии
        }

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar & dog_;
            ar & game_session_id_;
        }

        [[nodiscard]] std::shared_ptr<player::Player> Restore(const model::SessionService& session_manager) const {
        auto session = session_manager.FindGameSession(game_session_id_);
        if (!session) {
            throw std::runtime_error("GameSession not found for Player!");
        }

        // Проверяем, есть ли уже такая собака
        auto existing_dog = session->GetDogById(dog_.Restore().GetId());
        if (existing_dog) {
            return std::make_shared<player::Player>(existing_dog, session);
        }

        // Если собака отсутствует, создаём её
        auto dog = std::make_shared<model::Dog>(dog_.Restore());
        session->AddDog(dog);

        return std::make_shared<player::Player>(dog, session);
    }


    private:
        serialization::DogSer dog_;
        model::GameSession::Id game_session_id_;
    };

    class PlayerTokensSer {
    public:
        PlayerTokensSer() = default;
        
        explicit PlayerTokensSer(const app::PlayerTokens& player_tokens) {
            for (const auto& [token, player] : player_tokens.GetTokenMap()) {
                tokens_.emplace_back(token, player->GetDogId());
            }
        }

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar & tokens_;
        }

        [[nodiscard]] std::unordered_map<app::Token, std::shared_ptr<player::Player>, util::TaggedHasher<app::Token>>
            Restore(const app::Players& players) const {
            std::unordered_map<app::Token, std::shared_ptr<player::Player>, util::TaggedHasher<app::Token>> restored_tokens;

            for (const auto& [token, dog_id] : tokens_) {
                auto player = players.FindPlayerByDogId(model::Dog::Id(dog_id));
                if (player) {
                    restored_tokens[token] = player;
                }
            }

            return restored_tokens;
        }

    private:
        std::vector<std::pair<app::Token, uint64_t>> tokens_;
    };

    class PlayersSer {
    public:
        PlayersSer() = default;
        
        PlayersSer(const app::Players& players) {
            for (const auto& [dog_id, player] : players.GetPlayers()) {
                players_.emplace_back(*player);
                dog_ids_.push_back(dog_id);
            }
        }

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar & players_;
            ar & dog_ids_;
        }

        [[nodiscard]] std::unique_ptr<app::Players> Restore(const model::SessionService& session_manager) const {
            auto players = std::make_unique<app::Players>();
            
            for (size_t i = 0; i < players_.size(); ++i) {
                auto player = 
                    players_[i].Restore(session_manager);

                auto token = players->FindTokenByPlayer(player);
                if (*token == "") {
                    token = players->Add(player->GetDog(), 
                        player->GetGameSession());
                }
            }

            return players;
        }


    private:
        std::vector<PlayerSer> players_;
        std::vector<uint64_t> dog_ids_;
    };

    struct GameSave {
        serialization::GameSer game;
        PlayersSer players;

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar & game;
            ar & players;
        }

        [[nodiscard]] std::pair<model::Game, std::unique_ptr<app::Players>> Restore(model::SessionService& session_service) const {
            model::Game restored_game = game.Restore();
            std::unique_ptr<app::Players> restored_players = players.Restore(session_service);

            return {std::move(restored_game), std::move(restored_players)};
        }
    };

    
}