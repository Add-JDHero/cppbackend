#include "application.h"
#include "database_manager.h"
#include "handlers.h"
#include "json_loader.h"
#include "model.h"
#include "model_serialization.h"
#include "player.h"
#include "type_declarations.h"
#include "log.h"

#include <boost/log/trivial.hpp>
#include <boost/exception/all.hpp>

#include <boost/json/object.hpp>
#include <iomanip>
#include <exception>
#include <stdexcept>
#include <unordered_set>

namespace app {
    char to_uppercase(unsigned char c) {
        return std::toupper(c);
    }
    
    void to_uppercase_inplace(char& c) {
        c = to_uppercase(c);
    }

	Token PlayerTokens::GenerateToken() {
        std::ostringstream ss;
        ss << std::hex << std::setfill('0') 
            << std::setw(16) << generator1_()
            << std::setw(16) << generator2_();

        std::string token = ss.str();

        // Генерация символов a-f как в верхнем, так и в нижнем регистре
        std::mt19937 gen(random_device_());
        std::uniform_int_distribution<> dist(0, 1);

        std::transform(token.cbegin(), 
                       token.cend(), 
                       token.begin(), 
                       [&dist, &gen](char c) {
            if (std::isalpha(c) && dist(gen)) {
                c = std::toupper(c);
            }

            return c;
        });

        return Token{token};
    }

    std::shared_ptr<player::Player> 
    PlayerTokens::FindPlayerByToken(const Token& token) const {
        auto player = token_to_player_.find(token);
        if (player != token_to_player_.end())
            return player->second;

        return nullptr;
    }
    
    void PlayerTokens::RemovePlayer(Token player_token) {
        token_to_player_.erase(player_token);
    }

    bool Application::HasPlayerByName(const std::string& name) const {
        return players_.FindPlayerByName(name).has_value();
    }

    Application::PlayerInfo 
    Application::GetPlayerInfo(const std::string& name) { 
        if (!players_.FindPlayerByName(name).has_value()) {
            return {};
        }

        PlayerInfo pi;
        pi.token = players_.FindPlayerByName(name).value();
        pi.id = players_.GetPlayerByToken(pi.token)->GetDogId();
        
        return pi; 
    }

    Application::PlayerInfo 
    Application::ChangePlayerSession(std::shared_ptr<player::Player> player, 
                                     std::shared_ptr<model::GameSession> session) {
        RemovePlayerFromSession(player);

        return CreateNewPlayer(player->GetDogName(), session);                                       
    }

    Application::PlayerInfo 
    Application::AddPlayer(const std::string& player_name, 
                           std::shared_ptr<model::GameSession> session) {
        bool has_player = HasPlayerByName(player_name);

        if (has_player) {
            auto player = 
                players_.GetPlayerByToken(players_.FindPlayerByName(player_name).value());
            auto player_session = player->GetGameSession();
            auto session_id = session->GetSessionId();
            auto player_session_id = player_session->GetSessionId();
            if (session_id != player_session_id) {
                return ChangePlayerSession(player, session);
            } else {
                return GetPlayerInfo(player_name);
            }
        }

        return CreateNewPlayer(player_name, session);
    }

    Players::PlayerNameToPlayerId Players::GetPlayerNamesToId() const {
        PlayerNameToPlayerId data;
        for (const auto& [pair, player] : players_) {
            data[player->GetDogName()] = player->GetDogId();
        }

        return data;
    }

    Token Players::FindTokenByPlayer(std::shared_ptr<player::Player> player) { 
        return player_tokens_.FindTokenByPlayer(player);
    }

    Token Application::FindTokenByPlayer(std::shared_ptr<player::Player> player) {
        return players_.FindTokenByPlayer(player);
    }

    Token PlayerTokens::FindTokenByPlayer(std::shared_ptr<player::Player> player) const {
        for (const auto& [token, stored_player] : token_to_player_) {
            if (stored_player->GetDogName() == player->GetDogName()) {
                return token;
            }
        }

        return {};
    }

    void Application::RemovePlayerFromSession(std::shared_ptr<player::Player> player) {
        auto player_session = player->GetGameSession();
        player_session->RemoveDog(player->GetDogId());
        players_.Remove(player->GetDogId(), player_session->GetMapId());
    }

    Application::PlayerInfo 
    Application::CreateNewPlayer(const std::string& player_name, 
                                 std::shared_ptr<model::GameSession> session) {

        auto dog = std::make_shared<model::Dog>(player_name);
        dog->SetDefaultDogSpeed(session->GetMapDefaultSpeed());
        auto token = players_.Add(dog, session);
        
        PlayerInfo pi;

        pi.id = dog->GetId();
        pi.token = std::move(token);

        return pi;
    }

    std::shared_ptr<player::Player> 
    Players::GetPlayerByToken(const Token& token) const {
        return player_tokens_.FindPlayerByToken(token);
    }

    Token PlayerTokens::AddPlayer(std::shared_ptr<player::Player> player) {
        Token token = GenerateToken();
        token_to_player_[token] = player;

        return token;
    }

    Token Players::Add(std::shared_ptr<model::Dog> dog, 
                       std::shared_ptr<model::GameSession> game_session) {
        std::shared_ptr<player::Player> player = 
            std::make_shared<player::Player>(dog, game_session);
        Token token = player_tokens_.AddPlayer(player);
        players_[{dog->GetId(), *(game_session->GetMapId())}] = player;
    
        return token;
    }

    std::optional<Token> 
    Players::FindPlayerByName(const std::string& name) const {
        std::optional<Token> result;
        for (const auto& [personal_data, player] : players_) {
            if (player->GetDogName() == name) {
                result.emplace(player_tokens_.FindTokenByPlayer(player));
            }
        }

        return result;
    }

    void Players::Remove(model::Dog::Id dog_id, model::Map::Id map_id) {
        auto it = players_.find({dog_id, *map_id});
        auto token = FindTokenByPlayer(it->second);

        player_tokens_.RemovePlayer(token);
        if (it != players_.end()) {
            players_.erase(it);
        }
    }

Application::Application(model::Game& game, db::ConnectionPool& pool) 
        : game_(game)
        , connection_pool_(pool) {
    }

    const std::vector<std::string> 
    Application::GetPlayersList(const Token& token) const {
        auto player = 
            players_.GetPlayerByToken(token);
        if (!player) return {};

        auto game_session = player->GetGameSession();
        auto players_list = 
            game_session->GetPlayersNames();

        std::sort(players_list.begin(), players_list.end());

        return players_list;
    }

    void Application::SetApplicationListener(ApplicationListener& listener) {
        listener_ = &listener;
    }

    const std::string Application::GetSerializedPlayersList(const Token& token) const {
        boost::json::object players_json;

        auto player = 
            players_.GetPlayerByToken(token);
        if (!player) return boost::json::serialize(players_json);

        // for (const auto& )

        for (const auto& [dog_name, dog_id] : players_.GetPlayerNamesToId()) {
            players_json[std::to_string(dog_id)] = boost::json::object{
                {"name", dog_name}
            };
        }

        return boost::json::serialize(players_json);
    }

    bool Application::HasPlayerToken(Token token) const {
        auto player = 
            players_.GetPlayerByToken(app::Token{token});

        return player != nullptr;
    }

    const std::string 
    Application::GetSerializedGameState(const Token& token) const {
        auto game_session = players_.GetPlayerByToken(token)->GetGameSession();
        std::vector<model::State> states = game_session->GetPlayersUnitStates();
        const model::GameSession::LostObjects 
            lost_objects = game_session->GetLostObjects();

        return json_loader::StateSerializer::SerializeStates(states, 
                                                             lost_objects);
    }
    
    void Application::MovePlayer(const Token& token, std::string direction) {
        auto player = players_.GetPlayerByToken(token);
        player->MovePlayer(direction);
    }

    std::unique_ptr<app_serialization::PlayersSer> 
    Application::SerializePlayers() const {
        return std::make_unique<app_serialization::PlayersSer>(players_);
    }

    void Application::LoadPlayersFromFile(Players&& players) {
        players_.RestorePlayers(std::move(players));
    }

    void Application::LoadGameFromFile(model::Game game) {
        game_.LoadGameData(std::move(game.GetCommonData()), game_.GetLootService().GetGeneratorConfig());
        game_.SetDefaultDogSpeed(game.GetDefaultDogSpeed());
        game_.SetDefaultTickTime(game.GetDefaultTickTime());
    }

    std::vector<Application::PlayerMovementInfo> Application::PlayersInfoSnapstot() const {
        std::vector<PlayerMovementInfo> result;
        result.reserve(players_.GetPlayers().size());

        for (const auto& player : players_.GetPlayers()) {
            result.push_back({   
                    player.second->GetDogName(), 
                    player.second->GetDogPos(),
                    player.second->GetDogSpeed(),
                });
        }

        return result;
    }

    void Application::LoadGameFromFilie() {
        listener_->LoadStateFromFile();
    }

    bool Application::IsMoving(Application::PlayerMovementInfo& old, 
                               Application::PlayerMovementInfo& current) const {
        // bool is_moving = current.speed.x != 0 || current.speed.y != 0; 
        // return is_moving || old.pos != current.pos;
        return 0;
    }

    void Application::RemoveAFKPlayer(std::shared_ptr<player::Player> player) {
        auto dog_id = player->GetDogId();
        auto map_id = player->GetGameSession()->GetMapId();
        players_.Remove(dog_id, map_id);
    }

    bool Application::IsAFK(const Application::PlayerMovementInfo& old, 
                            const Application::PlayerMovementInfo& current) const {
        constexpr double EPSILON = 1e-6;

        bool is_not_moving = std::abs(current.speed.x) < EPSILON && 
                            std::abs(current.speed.y) < EPSILON;

        bool positions_equal = std::abs(old.pos.x - current.pos.x) < EPSILON &&
                            std::abs(old.pos.y - current.pos.y) < EPSILON;

        return is_not_moving || positions_equal;
    }

    void Application::SavePlayerStatsToDB(std::shared_ptr<model::Dog> dog) {
        try {
            auto connection = connection_pool_.GetConnection();

            pqxx::work work{*connection};
            
            work.exec_prepared("insert_retire",
                dog->GetName(),
                dog->GetState().score,
                dog->CalcPlayTime()
            );

            work.commit();
        } catch (const std::exception& e) {
            throw std::runtime_error(e.what());
        }
    }

    void Application::RemoveAFKPlayers(double delta_time,
                                       std::vector<PlayerMovementInfo>& old,
                                       std::vector<PlayerMovementInfo>& current) {
        if (old.size() != current.size()) { 
            throw std::logic_error("different size of players info (application layer)");
        }

        std::vector<std::shared_ptr<player::Player>> afk_players;

        for (auto old_it = old.begin(), current_it = current.begin(); 
             old_it != old.end() && current_it != current.end();
             ++old_it, ++current_it) {

            auto player_token = 
                players_.FindPlayerByName(old_it->dog_name.data());
            auto player = players_.GetPlayerByToken(player_token.value());
            
            if (IsAFK(*old_it, *current_it)) {                
                auto play_time = player->GetLastMoveTime();
                play_time += delta_time;
                player->SetLastMoveTime(play_time);

                if (play_time > dog_retirement_time_) {
                    SavePlayerStatsToDB(player->GetDog());

                    afk_players.push_back(player);
                }
            } else {
                player->SetLastMoveTime(0);
            }
        }

        for (auto& player : afk_players) {
            RemoveAFKPlayer(player);
        }
    }

    void Application::Tick(milliseconds delta_time) {
        auto old = PlayersInfoSnapstot();
        game_.GetEngine().Tick(delta_time);
        auto current = PlayersInfoSnapstot();

        RemoveAFKPlayers(delta_time.count(), old, current);

        if (listener_) {
            listener_->OnTick(delta_time);
        }
    } 
}

namespace serialization {

    SerializingListener::SerializingListener(app::Application& app, 
                                             const std::string& state_file, 
                                             milliseconds save_period)
        : app_(app)
        , state_file_(state_file)
        , save_period_(save_period) {
    }

    void SerializingListener::OnTick(milliseconds delta) {
        time_since_last_save_ += delta;

        if (time_since_last_save_ >= save_period_) {
            SaveStateToFile();
            time_since_last_save_ = milliseconds{0};
        }
    }

    void SerializingListener::SaveStateToFile() {
        std::stringstream ss;
        try {
            boost::archive::text_oarchive oa{ss};
            app_serialization::GameSave g_s;
            g_s.game = app_.SerializeGame();
            g_s.players = std::move(*app_.SerializePlayers());
            // oa << serialized_game;
            oa << g_s;

            std::string temp_file = state_file_ + ".tmp";
            {
                std::ofstream ofs(temp_file);
                if (!ofs) {
                    throw std::runtime_error("Failed to open temporary state file for writing.");
                }
                ofs << ss.str();
            }

            std::filesystem::rename(temp_file, state_file_);
            std::cout << "Game state saved to file: " << state_file_ << std::endl;
        } catch (const std::exception& e) {
            throw std::runtime_error(e.what());
        }
    }

    void SerializingListener::LoadGameDataFromFile(app_serialization::GameSave&& saved_game) {
        app_.LoadGameFromFile(std::move(saved_game.game.Restore()));
        auto tmp = 
            saved_game.players.Restore(
                app_.GetSessionService()
            );
        app_.LoadPlayersFromFile(std::move(*tmp));

    }

    void SerializingListener::LoadStateFromFile() {
        try {
            std::ifstream ifs(state_file_);
            if (!ifs) {
                throw std::runtime_error("Failed to open state file for reading.");
            }

            boost::archive::text_iarchive ia{ifs};

            app_serialization::GameSave g_s;
            ia >> g_s;
            
            LoadGameDataFromFile(std::move(g_s));

            std::cout << "Game state restored from file: "s << state_file_ << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error loading game state: " << e.what() << std::endl;
            exit(EXIT_FAILURE);
        }
    }

}
