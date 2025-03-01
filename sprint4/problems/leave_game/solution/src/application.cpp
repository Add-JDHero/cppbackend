#include "application.h"
#include "handlers.h"
#include "json_loader.h"
#include "model.h"
#include "player.h"
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

        return Token{token};
    }

    std::shared_ptr<Player::Player> PlayerTokens::FindPlayerByToken(const Token& token) const {
        auto player = token_to_player_.find(token);
        if (player != token_to_player_.end())
            return player->second;

        return nullptr;
    }

    Token Application::AddPlayer(std::shared_ptr<model::Dog> dog, 
                                 std::shared_ptr<model::GameSession> session) {
        auto existing_player = 
            FindExistingPlayer(dog->GetId(), session->GetMapId());

        if (existing_player) {
            return HandleExistingPlayer(existing_player, session);
        }

        return CreateNewPlayer(dog, session);
    }

    Token Players::FindTokenByPlayer(std::shared_ptr<Player::Player> player) { 
        return player_tokens_.FindTokenByPlayer(player);
    }

    Token Application::FindTokenByPlayer(std::shared_ptr<Player::Player> player) {
        return players_.FindTokenByPlayer(player);
    }

    std::shared_ptr<Player::Player> 
    Application::FindExistingPlayer(model::Dog::Id dog_id, 
                                    model::Map::Id map_id) {
        return players_.FindByDogAndMapId(dog_id, map_id);
    }


    Token Application::HandleExistingPlayer(std::shared_ptr<Player::Player> player, 
                                        std::shared_ptr<model::GameSession> new_session) {
        auto existing_session = player->GetGameSession();

        if (existing_session->GetMapId() == new_session->GetMapId()) {
            return players_.FindTokenByPlayer(player);
        }

        std::string dog_name = player->GetGameSession()->GetDogs().at(player->GetDogId())->GetName();

        RemovePlayerFromSession(player, existing_session);
        return CreateNewPlayer(std::make_shared<model::Dog>(dog_name), new_session);
    }


    void Application::RemovePlayerFromSession(std::shared_ptr<Player::Player> player, 
                                          std::shared_ptr<model::GameSession> session) {
        session->RemoveDog(player->GetDogId());
        players_.Remove(player->GetDogId(), session->GetMapId());
    }

    Token Application::CreateNewPlayer(std::shared_ptr<model::Dog> dog, 
                                   std::shared_ptr<model::GameSession> session) {
        dog->SetDefaultDogSpeed(session->GetMapDefaultSpeed());
        session->AddDog(dog);
        return players_.Add(dog, session);
    }

    std::shared_ptr<Player::Player> Players::GetPlayerByToken(const Token& token) const {
        return player_tokens_.FindPlayerByToken(token);
    }

    std::shared_ptr<Player::Player> Players::FindByDogAndMapId(model::Dog::Id dog_id, 
                                                           model::Map::Id map_id) {
        auto it = players_.find({dog_id, *map_id});
        return it != players_.end() ? it->second : nullptr;
    }

    Token PlayerTokens::AddPlayer(std::shared_ptr<Player::Player> player) {
        Token token = GenerateToken();
        token_to_player_[token] = player;

        return token;
    }

    Token Players::Add(std::shared_ptr<model::Dog> dog, 
                       std::shared_ptr<model::GameSession> game_session) {
        std::shared_ptr<Player::Player> player = std::make_shared<Player::Player>(dog, game_session);
        Token token = player_tokens_.AddPlayer(player);
        players_[{dog->GetId(), *(game_session->GetMapId())}] = 
            std::make_shared<Player::Player>(dog, game_session);
    
        return token;
    }

    void Players::Remove(model::Dog::Id dog_id, model::Map::Id map_id) {
        auto it = players_.find({dog_id, *map_id});
        if (it != players_.end()) {
            players_.erase(it);
        }
    }

    Application::Application(model::Game& game) 
        : game_(game) {
    }

    const std::vector<std::string> Application::GetPlayersList(const Token& token) const {
        auto player = players_.GetPlayerByToken(token);
        if (!player) return {};

        auto game_session = player->GetGameSession();
        auto players_list = game_session->GetPlayersNames();

        std::sort(players_list.begin(), players_list.end());

        return players_list;
    }

    void Application::SetApplicationListener(ApplicationListener& listener) {
        listener_ = &listener;
    }

    const std::string Application::GetSerializedPlayersList(const Token& token) const {
        std::vector<std::string> names = GetPlayersList(token);
        boost::json::object players_json;
        
        int index = 0;
        for (const auto& player_name : names) {
            players_json[std::to_string(index)] = boost::json::object{{"name", player_name}};
            index++;
        }

        return boost::json::serialize(players_json);
    }


    bool Application::HasPlayerToken(Token token) const {
        auto player = players_.GetPlayerByToken(app::Token{token});

        return player != nullptr;
    }

    const std::string Application::GetSerializedGameState(const Token& token) const {
        auto game_session = players_.GetPlayerByToken(token)->GetGameSession();
        std::vector<model::State> states = game_session->GetPlayersUnitStates();
        const model::GameSession::LostObjects 
            lost_objects = game_session->GetLostObjects();

        return json_loader::StateSerializer::SerializeStates(states, lost_objects);
    }
    
    void Application::MovePlayer(const Token& token, std::string direction) {
        auto player = players_.GetPlayerByToken(token);
        player->MovePlayer(direction);
    }

    void Application::Tick(milliseconds delta_time) const {
        game_.GetEngine().Tick(delta_time);

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
        GameSer serialized_game = app_.SerializeGame();
        oa << serialized_game;

        std::string temp_file = state_file_ + ".tmp";
        {
            std::ofstream ofs(temp_file);
            if (!ofs) {
                throw std::runtime_error("Failed to open temporary state file for writing.");
            }
            ofs << ss.str();
        }

        std::filesystem::rename(temp_file, state_file_);
        std::cout << "Game state saved to " << state_file_ << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error saving game state: " << e.what() << std::endl;
    }
}

void SerializingListener::LoadStateFromFile() {
    try {
        std::ifstream ifs(state_file_);
        if (!ifs) {
            throw std::runtime_error("Failed to open state file for reading.");
        }

        boost::archive::text_iarchive ia{ifs};
        GameSer serialized_game;
        ia >> serialized_game;
        
        app_.LoadGameFromFile(std::move(serialized_game.Restore()));

        std::cout << "Game state restored from " << state_file_ << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error loading game state: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}

}
