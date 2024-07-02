#include "application.h"
#include "handlers.h"
#include "json_loader.h"
#include "type_declarations.h"
#include <boost/json/object.hpp>
#include <iomanip>


namespace app {
    char to_uppercase(unsigned char c) {
        return std::toupper(c);
    }
    
    void to_uppercase_inplace(char& c) {
        c = to_uppercase(c);
    }

	Token PlayerTokens::GenerateToken() {
        std::ostringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(16) << generator1_()
        << std::setw(16) << generator2_();

        std::string token = ss.str();

        // Генерация символов a-f как в верхнем, так и в нижнем регистре
        std::mt19937 gen(random_device_());
        std::uniform_int_distribution<> dist(0, 1);

        std::transform(token.cbegin(), token.cend(), token.begin(), [&dist, &gen](char c) {
            if (std::isalpha(c) && dist(gen)) {
                c = std::toupper(c);
            }

            return c;
        });

        /* for (char& c : token) {
            if (std::isalpha(c) && dist(gen)) {
                c = std::toupper(c);
            }
        } */

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

    std::shared_ptr<Player> Players::GetPlayerByToken(const Token& token) const {
        return player_tokens_.FindPlayerByToken(token);
    }

    std::shared_ptr<Player> Players::FindByDogAndMapId(model::Dog::Id dog_id, 
                                                        model::Map::Id map_id) {
        return players_[{dog_id, *map_id}];
    }



    Application::Application(model::Game& game) 
        : game_(game) {
    }

    const std::vector<std::string> Application::GetPlayersList(const Token& token) const {
        auto game_session = players_.GetPlayerByToken(token)->GetGameSession();            
        return game_session->GetPlayersNames();
    }

    const std::string Application::GetSerializedPlayersList(const Token& token) const {
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

    const std::string Application::GetSerializedGameState(const Token& token) const {
        auto game_session = players_.GetPlayerByToken(token)->GetGameSession();
        std::vector<model::State> states = game_session->GetPlayersUnitStates();

        return json_loader::StateSerializer::SerializeStates(states);
    }

    
    void Application::MovePlayer(const Token& token, std::string direction) {
        auto player = players_.GetPlayerByToken(token);
        player->MovePlayer(direction);
    }

    void Application::Tick(double delta_time) const {
        game_.Tick(delta_time / 1000);
    } 
}