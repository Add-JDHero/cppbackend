#include "model.h"

#include <stdexcept>
#include <cmath>
#include <cassert>

namespace model {
using namespace std::literals;


    Road::Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road::Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool Road::IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool Road::IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point Road::GetStart() const noexcept {
        return start_;
    }

    Point Road::GetEnd() const noexcept {
        return end_;
    }


    Map::Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Map::Id& Map::GetId() const noexcept {
        return id_;
    }

    void Map::SetDefaultDogSpeed(int64_t default_speed) {
        default_dog_speed_ = default_speed; 
    }
    
    double Map::GetDefaultDogSpeed() const {
        return default_dog_speed_;
    }

    bool Map::IsDefaultDogSpeedValueConfigured() const {
        return default_dog_speed_ != 1;
    }

    const std::string& Map::GetName() const noexcept {
        return name_;
    }

    const Map::Buildings& Map::GetBuildings() const noexcept {
        return buildings_;
    }

    const Map::Roads& Map::GetRoads() const noexcept {
        return roads_;
    }

    const Map::Offices& Map::GetOffices() const noexcept {
        return offices_;
    }

    void Map::AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void Map::AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void Map::AddOffice(Office office) {
        if (warehouse_id_to_index_.contains(office.GetId())) {
            throw std::invalid_argument("Duplicate warehouse");
        }

        const size_t index = offices_.size();
        Office& o = offices_.emplace_back(std::move(office));
        try {
            warehouse_id_to_index_.emplace(o.GetId(), index);
        } catch (...) {
            // Удаляем офис из вектора, если не удалось вставить в unordered_map
            offices_.pop_back();
            throw;
        }
    }
    

    Dog::Dog(std::string_view name) : name_(std::string(name)) {
        state_.id = general_id_++;
    };

    const Dog::Id Dog::GetId() const {
        return state_.id;
    }
    const std::string& Dog::GetName() const {
        return name_;
    }

        const Pos& Dog::GetPosition() const noexcept {
        return state_.position;
    }

    const Speed& Dog::GetSpeed() const noexcept {
        return state_.speed;
    }

    const Direction& Dog::GetDirection() const noexcept {
        return state_.direction;
    }

    void Dog::SetSpeed(double x, double y) {
        state_.speed.x = x;
        state_.speed.y = y;
    }
    
    Pos Dog::MoveDog(Pos new_position) {
        state_.position = new_position;
        return state_.position;
    }

    void Dog::SetDefaultDogSpeed(double speed) {
        default_dog_speed_ = speed;
    }

    void Dog::StopDog() {
        state_.speed = {0, 0};
    }

    void Dog::SetDogDirSpeed(std::string dir) {
        if (dir == "") {
            SetSpeed(0, 0);
        } else if (dir == "L") {
            SetSpeed(-default_dog_speed_, 0);
            state_.direction = Direction::WEST;
        } else if (dir == "R") {
            SetSpeed(default_dog_speed_, 0);
            state_.direction = Direction::EAST;
        } else if (dir == "U") {
            SetSpeed(0, -default_dog_speed_);
            state_.direction = Direction::NORTH;
        } else if (dir == "D") {
            SetSpeed(0, default_dog_speed_);
            state_.direction = Direction::SOUTH;
        } else {
            assert(false);
        }

        return;
        
    }

    const State& Dog::GetState() const noexcept {
        return state_;
    }


    GameSession::GameSession(const Map& map) : map_(map), /* road_manager_(map.GetRoads()), */ id_(general_id_++) {
        InitializeRegions(map_, regions_);
    }

    Map::Id GameSession::GetMapId() const {
        return map_.GetId();
    }

    double GameSession::GetMapDefaultSpeed() const {
        return map_.GetDefaultDogSpeed();
    }

    GameSession::Id GameSession::GetSessionId() const {
        return general_id_;
    }

    void GameSession::AddDog(std::shared_ptr<Dog> dog) {
        dogs_.insert({dog->GetId(), dog});
        dogs_vector_.push_back(dog);
    }

    const GameSession::Dogs& GameSession::GetDogs() const {
        return dogs_;
    }

    const std::vector<std::string> GameSession::GetPlayersNames() const {
        std::vector<std::string> names;
        for (const auto& [dog_id, dog]: dogs_) {
            names.push_back(dog->GetName());
        }

        return names;
    }

    const std::vector<State> GameSession::GetPlayersUnitStates() const {
        std::vector<State> dogs;
        for (const auto& [dog_id, dog]: dogs_) {
            dogs.push_back(dog->GetState());
        }

        return dogs;
    }

    bool GameSession::HasDog(Dog::Id id) {
        return dogs_.count(id) > 0;
    }

    void GameSession::MovePlayer(Dog::Id id, double delta_time) {
        auto& dog = dogs_[id];
        auto new_position = CalculateNewPosition(dog->GetPosition(), dog->GetSpeed(), delta_time);

        if (IsWithinAnyRegion(new_position, regions_)) {
            dog->MoveDog(new_position);
        } else {
            Pos max_pos = AdjustPositionToMaxRegion(dog);
            dog->MoveDog(max_pos);
            dog->StopDog();
        }
    }

    Pos GameSession::AdjustPositionToMaxRegion(const std::shared_ptr<Dog>& dog) {
        Pos max_pos = dog->GetPosition();
        double max_diff = 0;

        for (const auto& [region_id, region] : regions_) {
            if (region.Contains(dog->GetPosition())) {
                Pos possible_max_pos = MaxValueOfRegion(region, dog->GetDirection(), dog->GetPosition());
                Pos max_distance_pos = MoveOnMaxDistance(dog->GetPosition(), possible_max_pos, max_diff);

                double dx = max_distance_pos.x - dog->GetPosition().x;
                double dy = max_distance_pos.y - dog->GetPosition().y;
                double distance = std::sqrt(dx * dx + dy * dy);

                if (distance > max_diff) {
                    max_diff = distance;
                    max_pos = max_distance_pos;
                }
            }
        }

        return max_pos;
    }

    Pos GameSession::MaxValueOfRegion(Region reg, Direction dir, Pos current_pos) {
        Pos result = current_pos;
        if (dir == Direction::EAST) {
            result.x = reg.max_x;
        } else if (dir == Direction::WEST) {
            result.x = reg.min_x;
        } else if (dir == Direction::SOUTH) {
            result.y = reg.max_y;
        } else if (dir == Direction::NORTH) {
            result.y = reg.min_y;
        }

        return result;
    }

    Pos GameSession::MoveOnMaxDistance(const Pos& current, const Pos& possible, double current_max) {
        double dx = current.x - possible.x;
        double dy = current.y - possible.y;
        double dist = std::sqrt(dx * dx + dy * dy);

        if (dist > current_max) {
            return possible;
        } 

        return current;
    }
    

    void GameSession::StopPlayer(Dog::Id id) {
        dogs_[id]->StopDog();
    }

    void GameSession::Tick(double delta_time) {
        for (const auto& [dog_id, dog]: dogs_) {
            MovePlayer(dog_id, delta_time);
        }

        /* for (const auto& dog: dogs_vector_) {
            MovePlayer(dog->GetId(), delta_time);
        } */
    }

    Pos GameSession::CalculateNewPosition(const Pos& position, const Speed& speed, double delta_time) {
        Pos result{};

        result.x = position.x + speed.x * delta_time;
        result.y = position.y + speed.y * delta_time;

        return result;
    }

    // Добавление дорог в регионы
    void GameSession::AddRoadToRegions(const Road& road, std::unordered_map<int, Region>& regions) {
        if (road.IsHorizontal()) {
            double left_x = road.GetStart().x;
            double right_x = road.GetEnd().x;
            if (left_x > right_x) { std::swap(left_x, right_x); }
            regions.emplace(regions.size(), Region{left_x - 0.4, right_x + 0.4, road.GetStart().y - 0.4, road.GetStart().y + 0.4});
        } else if (road.IsVertical()) {
            double lower_y = road.GetStart().y;
            double upper_y = road.GetEnd().y;
            if (lower_y > upper_y) { std::swap(lower_y, upper_y); }
            regions.emplace(regions.size(), Region{road.GetStart().x - 0.4, road.GetStart().x + 0.4, lower_y - 0.4, upper_y + 0.4});
        }
    }


    // Инициализация регионов
    void GameSession::InitializeRegions(const Map& map, std::unordered_map<int, Region>& regions) {
        for (const auto& road : map.GetRoads()) {
            AddRoadToRegions(road, regions);
        }
    }

    // Проверка, находится ли позиция в любой области
    bool GameSession::IsWithinAnyRegion(const Pos& pos, const std::unordered_map<int, Region>& regions) {
        for (const auto& [id, region] : regions) {
            if (region.Contains(pos)) {
                return true;
            }
        }
        return false;
    }

    // Корректировка позиции в рамках региона
    Pos GameSession::AdjustPositionToRegion(const Pos& position, const Region& region) {
        Pos adjusted = position;
        if (position.x > region.max_x) { adjusted.x = region.max_x; }
        if (position.x < region.min_x) { adjusted.x = region.min_x; }
        if (position.y > region.max_y) { adjusted.y = region.max_y; }
        if (position.y < region.min_y) { adjusted.y = region.min_y; }
        return adjusted;
    }



    void Game::AddMap(Map map) {
        const size_t index = maps_.size();
        if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
            throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
        } else {
            try {
                maps_.emplace_back(std::move(map));
            } catch (...) {
                map_id_to_index_.erase(it);
                throw;
            }
        }
    }

    const Game::Maps& Game::GetMaps() const noexcept {
        return maps_;
    }

    void Game::SetDefaultDogSpeed(double default_speed) {
        default_dog_speed_ = default_speed; 
    }

    double Game::GetDefaultDogSpeed() const {
        return default_dog_speed_;
    }

    std::shared_ptr<GameSession> Game::FindGameSession(Map::Id map_id) {
        if (!FindMap(map_id)) { return nullptr;}

        if (map_id_to_session_index_.count(map_id)) {
            return FindGameSessionBySessionId(map_id_to_session_index_[map_id]);
        }

        return CreateGameSession(map_id);
    }

    std::shared_ptr<GameSession> Game::CreateGameSession(Map::Id map_id) {
        auto result = std::make_shared<GameSession>(maps_[map_id_to_index_[map_id]]);
        int index = sessions_.size();
        sessions_.push_back(result);
        game_sessions_id_to_index_[result->GetSessionId()] = index;
        map_id_to_session_index_[map_id] = result->GetSessionId();

        return result;
    }

    void Game::Tick(double delta_time) {
        for (const auto& session: sessions_) {
            session->Tick(delta_time);
        }
    }

    const Map* Game::FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        
        return nullptr;
    }

    std::shared_ptr<GameSession> Game::FindGameSessionBySessionId(GameSession::Id session_id) {
        if (game_sessions_id_to_index_.count(session_id)) {
            return sessions_[game_sessions_id_to_index_[session_id]];
        }

        return nullptr;
    }

}  // namespace model
