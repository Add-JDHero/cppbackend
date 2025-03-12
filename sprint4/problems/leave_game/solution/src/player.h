#pragma once

#include "sdk.h"
#include "type_declarations.h"
#include "model.h"

#include <cstdint>
#include <memory>
#include <iostream>

namespace player {

    namespace detail {
        struct TokenTag {};
    }  

    using Token = util::Tagged<std::string, detail::TokenTag>;

    class Player {
    public:
        Player() = delete;
        Player(std::shared_ptr<model::Dog> dog, std::shared_ptr<model::GameSession> game_session);
        
        model::Dog::Id GetDogId();

        const std::string& GetDogName() const {
            return dog_->GetName();
        }

        void RemovePlayer() {
            game_session_->RemoveDog(dog_->GetId());
        }

        int GetDogScore() const { return dog_->GetState().score; }

        double GetLastMoveTime() const { return last_move_time_; }

        void SetLastMoveTime(double value) { last_move_time_ = value; }

        model::Speed GetDogSpeed() const { return dog_->GetSpeed(); }
        model::Pos GetDogPos() const { return dog_->GetPosition(); }

        void MovePlayer(std::string direction = "");

        int CalcPlayTime() const {
            std::chrono::time_point now = std::chrono::steady_clock::now();
            return std::chrono::duration_cast<std::chrono::duration<int>>(
                now - play_time_).count();
        }

        const std::shared_ptr<model::GameSession> GetGameSession() const;
        const std::shared_ptr<model::Dog> GetDog() const { return dog_; }

    private:
        double last_move_time_ = 0;

        std::chrono::steady_clock::time_point play_time_ = 
            std::chrono::steady_clock::now();

        std::shared_ptr<model::Dog> dog_;
        std::shared_ptr<model::GameSession> game_session_;
        
    };
}
