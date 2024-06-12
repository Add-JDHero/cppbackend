#include "player.h"
#include <iomanip>


namespace app {
	Token PlayerTokens::GenerateToken() {
    std::ostringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << generator1_()
       << std::setw(16) << std::setfill('0') << generator2_();
    return Token{ss.str()};
}

    Application::Application(model::Game& game, Players& players) 
        : game_(game), players_(players) {}

    const std::vector<std::string> Application::GetPlayersList(Token token) const {
        auto game_session = players_.GetPlayerByToken(token)->GetGameSession();            
        return game_session->GetPlayersNames();
    }
}