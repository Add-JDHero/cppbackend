#include "player.h"

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