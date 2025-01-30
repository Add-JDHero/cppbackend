#pragma once

#include "sdk.h"
#include "type_declarations.h"
#include "model.h"

#include <cstdint>
#include <memory>
#include <iostream>

namespace Player {

    namespace detail {
        struct TokenTag {};
    }  

    using Token = util::Tagged<std::string, detail::TokenTag>;

    class Player {
    public:
        Player() = delete;
        Player(std::shared_ptr<model::Dog> dog, std::shared_ptr<model::GameSession> game_session);
        model::Dog::Id GetDogId();

        void MovePlayer(std::string direction = "");

        const std::shared_ptr<model::GameSession> GetGameSession() const;

    private:
        std::shared_ptr<model::Dog> dog_;
        std::shared_ptr<model::GameSession> game_session_;
        
    };
}
