#include "player.h"
#include "json_loader.h"
#include <boost/json/object.hpp>
#include <iomanip>


namespace app {
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

    Application::Application(model::Game& game, Players& players) 
        : game_(game), players_(players) {}

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

    const std::string Application::GetSerializedGameState(Token token) const {
        auto game_session = players_.GetPlayerByToken(token)->GetGameSession();
        std::vector<model::State> states = game_session->GetPlayersUnitStates();

        return json_loader::StateSerializer::SerializeStates(states);
    }
}