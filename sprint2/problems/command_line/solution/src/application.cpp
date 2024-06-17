#include "application.h"
#include "handlers.h"
#include "json_loader.h"
#include "model.h"
#include "type_declarations.h"
#include <boost/json/object.hpp>
#include <iomanip>


namespace app {

    Player::Player(std::shared_ptr<model::Dog> dog, std::shared_ptr<model::GameSession> game_session):
        dog_(dog), 
        game_session_(game_session) {
        game_session->AddDog(dog_);
    }

    model::Dog::Id Player::GetDogId() {
        return dog_->GetId();
    }

    void Player::MovePlayer(std::string direction) {
        dog_->SetDogDirSpeed(direction);
    }

    const std::shared_ptr<model::GameSession> Player::GetGameSession() const {
        return game_session_;
    }


	Token PlayerTokens::GenerateToken() {
        std::ostringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(16) << generator1_()
        << std::setw(16) << generator2_();

        std::string token = ss.str();

        // Генерация символов a-f как в верхнем, так и в нижнем регистре
        std::mt19937 gen(random_device_());
        std::uniform_int_distribution<> dist(0, 1);

        for (char& c : token) {
            if (std::isalpha(c) && dist(gen)) {
                c = std::toupper(c);
            }
        }

        return Token{token};
    }

    Token PlayerTokens::AddPlayer(std::shared_ptr<Player> player) {
        Token token = GenerateToken();
        token_to_player_[token] = player;

        return token;
    }

    std::shared_ptr<Player> PlayerTokens::FindPlayerByToken(const Token& token) const {
        auto player = token_to_player_.find(token);
        if (player != token_to_player_.end())
            return player->second;

        return nullptr;
    }


    Token Players::Add(std::shared_ptr<model::Dog> dog, 
                       std::shared_ptr<model::GameSession> game_session) {
        std::shared_ptr<Player> player = std::make_shared<Player>(dog, game_session);
        Token token = player_tokens_.AddPlayer(player);
        players_[{dog->GetId(), *(game_session->GetMapId())}] = 
            std::make_shared<Player>(dog, game_session);
    
        return token;
    }

    std::shared_ptr<Player> Players::GetPlayerByToken(Token token) const {
        return player_tokens_.FindPlayerByToken(token);
    }

    std::shared_ptr<Player> Players::FindByDogAndMapId(model::Dog::Id dog_id, 
                                                        model::Map::Id map_id) {
        return players_[{dog_id, *map_id}];
    }



    Application::Application(std::filesystem::path config_path) {
        game_ = json_loader::LoadGame(config_path);
    }

    const std::vector<std::string> Application::GetPlayersList(Token token) const {
        auto game_session = players_.GetPlayerByToken(token)->GetGameSession();            
        return game_session->GetPlayersNames();
    }

    const std::string Application::GetSerializedPlayersList(Token token) const {
        std::vector<std::string> names = GetPlayersList(app::Token(token));
        boost::json::object players_json;
        int index = 0;
        
        for (const auto& player_name : names) {
            players_json[std::to_string(index)] = { {"name", player_name} };
            index++;
        }

        return boost::json::serialize(players_json);
    }

    bool Application::HasPlayerToken(Token token) const {
        auto player = players_.GetPlayerByToken(app::Token{token});

        return player != nullptr;
    }

    Token Application::AddPlayer(std::shared_ptr<model::Dog> dog, 
                                 std::shared_ptr<model::GameSession> session) {
        auto speed = session->GetMapDefaultSpeed();
        dog->SetDefaultDogSpeed(speed);
        
        return players_.Add(dog, session);
    }

    const std::string Application::GetSerializedGameState(Token token) const {
        auto game_session = players_.GetPlayerByToken(token)->GetGameSession();
        std::vector<model::State> states = game_session->GetPlayersUnitStates();

        return json_loader::StateSerializer::SerializeStates(states);
    }

    const model::Map* Application::FindMap(model::Map::Id map_id) const {
        return game_.FindMap(map_id);
    }

    const std::vector<model::Map>& Application::GetMaps() const {
        return game_.GetMaps();
    }

    std::shared_ptr<model::GameSession> 
    Application::FindGameSession(model::Map::Id map_id) {
        return game_.FindGameSession(map_id);
    }

    
    void Application::MovePlayer(Token token, std::string direction) {
        auto player = players_.GetPlayerByToken(token);
        player->MovePlayer(direction);
    }

    void Application::Tick(double delta_time) {
        game_.Tick(delta_time / 1000);
    } 
}