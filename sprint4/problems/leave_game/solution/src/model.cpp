#include "model.h"
#include "database_manager.h"
#include "extra_data.h"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <cmath>
#include <cassert>
#include <iostream>
#include <boost/log/trivial.hpp>
#include <boost/exception/all.hpp>

// namespace keywords = boost::log::keywords;
// namespace expr = boost::log::expressions;

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
    
    double Map::GetDefaultDogSpeed() const noexcept {
        return default_dog_speed_;
    }

    bool Map::IsDefaultDogSpeedValueConfigured() const noexcept {
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

    void Map::SetBagCapacity(int capacity) { 
        bag_capacity_ = capacity; 
    }

    int Map::GetBagCapacity() const noexcept { 
        return bag_capacity_; 
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
    
    Dog::Dog(State state, std::string name)
        : state_(std::move(state))
        , name_(std::move(name)) {
        
    }

    Dog::Dog(std::string_view name) : name_(std::string(name)) {
        state_.id = general_id_++;
    };

    const Dog::Id Dog::GetId() const noexcept {
        return state_.id;
    }
    const std::string& Dog::GetName() const noexcept {
        return name_;
    }

        const Pos& Dog::GetPosition() const noexcept {
        return state_.position;
    }

    const Speed& Dog::GetSpeed() const noexcept {
        return state_.speed;
    }

    const double Dog::GetDefaultDogSpeed() const noexcept {
         return default_dog_speed_; 
    }

    void Dog::AddToBag(uint64_t loot_id, uint64_t loot_type) {
        if (state_.bag.size() < bag_capacity_) {
            state_.bag.push_back({.id = loot_id, .type = loot_type});
        }
    }

    void Dog::ClearBag() { 
        state_.bag.clear(); 
    }

    const std::vector<FoundObject>& Dog::GetBag() const noexcept {
        return state_.bag;
    }

    void Dog::SetRandomPosition(Pos pos) { 
        state_.position = pos; 
    }

    void Dog::SetBagCapacity(size_t capacity) {
        bag_capacity_ = capacity;
    }

    const Direction& Dog::GetDirection() const noexcept {
        return state_.direction;
    }

    void Dog::AddScore(int score) { 
        state_.score += score; 
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

    void SessionService::ConfigureSessionData(std::shared_ptr<GameSession> session) {
        int index = common_data_->sessions_.size();

        common_data_->sessions_.push_back(session);
        common_data_->game_sessions_id_to_index_[session->GetSessionId()] = index;
        common_data_->mapId_to_session_index_[session->GetMapId()] = 
            session->GetSessionId();
    } 

    std::shared_ptr<GameSession> SessionService::CreateGameSession(Map::Id map_id) {
        auto& map = common_data_->maps_[common_data_->map_id_to_index_[map_id]];

        // Наполняем lootId_to_value_
        std::unordered_map<int, int> type_to_loot_values;
        auto it = common_data_->mapId_to_lootTypes_.find(map_id);
        int item_type = 0;
        if (it != common_data_->mapId_to_lootTypes_.end()) {
            const auto& loot_array = *it->second;
            for (const auto& item : loot_array) {
                int value = item.at("value").as_int64();
                type_to_loot_values[item_type++] = value;
            }
        }

        // Создаём GameSession с lootId_to_value_
        auto result = std::make_shared<GameSession>(map, std::move(type_to_loot_values));

        ConfigureSessionData(result);

        return result;
    }

    GameSession::GameSession(const Map& map, 
                             std::unordered_map<int, int> loot_values,  bool is_deserialized)
        : map_(map)
        , bag_capacity_(map.GetBagCapacity())
        , lootId_to_value_(std::move(loot_values)) {
            if (!is_deserialized) {
                id_= general_id_;
            }
            general_id_++;
        InitializeRegions(map_, regions_);
    }


    Map::Id GameSession::GetMapId() const noexcept {
        return map_.GetId();
    }

    double GameSession::GetMapDefaultSpeed() const noexcept {
        return map_.GetDefaultDogSpeed();
    }

    GameSession::Id GameSession::GetSessionId() const noexcept {
        return id_;
    }

    void GameSession::AddDog(std::shared_ptr<Dog> dog, bool is_deserialized) {
        if (dogs_.count(dog->GetId()) == 0) {
            if (!is_deserialized) {
                dog->SetRandomPosition(GenerateRandomRoadPosition());
            }
            dogs_.insert({dog->GetId(), dog});
            dogs_vector_.push_back(dog);
        }
    }

    const GameSession::Dogs& GameSession::GetDogs() const noexcept {
        return dogs_;
    }

    const std::vector<std::string> GameSession::GetPlayersNames() const noexcept {
        std::vector<std::string> names;
        for (const auto& [dog_id, dog]: dogs_) {
            names.push_back(dog->GetName());
        }

        return names;
    }

    const std::vector<State> GameSession::GetPlayersUnitStates() const noexcept {
        std::vector<State> dogs;
        for (const auto& dog: dogs_vector_) {
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

    std::vector<collision_detector::Gatherer> GameSession::GetGatherers(double delta_time) const {
        std::vector<collision_detector::Gatherer> gatherers;
        for (const auto& dog : dogs_vector_) {
            geom::Point2D start(dog->GetPosition().x, dog->GetPosition().y);
            geom::Point2D end(start.x + dog->GetSpeed().x * delta_time, 
                            start.y + dog->GetSpeed().y * delta_time);
            
            gatherers.push_back({start, end, 0.6});  // Ширина собаки = 0.6
        }
        return gatherers;
    }

    std::vector<collision_detector::Item> GameSession::GetItems() const {
        std::vector<collision_detector::Item> items;
        for (const auto& loot : loots_) {
            items.push_back({
                geom::Point2D(loot.position.x, loot.position.y), // Координаты предмета
                0.0 // Ширина предмета (нулевая)
            });
        }
        return items;
    }

    std::unordered_map<Dog::Id, Pos> GameSession::ComputeNewPositions(double delta_time) {
        std::unordered_map<Dog::Id, Pos> new_positions;
        for (const auto& [dog_id, dog] : dogs_) {
            new_positions[dog_id] = CalculateNewPosition(dog->GetPosition(), dog->GetSpeed(), delta_time);
        }
        return new_positions;
    }

    std::vector<collision_detector::GatheringEvent> 
    GameSession::DetectGatheringEvents(double delta_time) {
        using namespace collision_detector;
        return FindGatherEvents(
            VectorItemGathererProvider(GetItems(), 
            GetGatherers(delta_time)));
    }

    std::unordered_set<size_t> 
    GameSession::ProcessLootCollection(const std::vector<collision_detector::GatheringEvent>& events, 
                                       double delta_time) {
        std::unordered_set<size_t> collected_loot_ids;
        double last_time = 0.0;

        for (const auto& event : events) {
            double event_real_time = event.time * delta_time;
            double delta = event_real_time - last_time;

            for (const auto& [dog_id, dog] : dogs_) {
                MovePlayer(dog_id, delta);
            }

            if (event.item_id < loots_.size() && collected_loot_ids.count(loots_[event.item_id].id) == 0) {
                auto& player = dogs_vector_.at(event.gatherer_id);
                if (player->GetBag().size() < bag_capacity_) {
                    int loot_id = loots_[event.item_id].id;
                    int loot_type = loots_[event.item_id].type;
                    player->AddToBag(loot_id, loot_type);

                    // std::cerr << "[DEBUG] Player " << event.gatherer_id
                    //     << " collected loot ID: " << loots_[event.item_id].id
                    //     << " (Index: " << event.item_id << ")" << std::endl;

                    collected_loot_ids.insert(loots_[event.item_id].id);
                }
            }

            last_time = event_real_time;
        }

        return collected_loot_ids;
    }

    void GameSession::RemoveCollectedLoot(const std::unordered_set<size_t>& collected_loot_ids) {
        std::vector<LostObject> new_loots;
        for (const auto& loot : loots_) {
            if (!collected_loot_ids.count(loot.id)) {
                new_loots.push_back(loot);
            } else {
                std::cerr << "[DEBUG] Removing loot ID: " << loot.id << std::endl;
            }
        }
        loots_ = std::move(new_loots);
    }

    void GameSession::ProcessLootDelivery() {
        for (auto& [dog_id, dog] : dogs_) {
            for (const auto& office : map_.GetOffices()) {
                double dist = std::sqrt(
                    std::pow(dog->GetPosition().x - office.GetPosition().x, 2) +
                    std::pow(dog->GetPosition().y - office.GetPosition().y, 2)
                );

                if (dist <= (0.5 / 2 + 0.6 / 2)) { // Условие сдачи предметов
                    int total_score = 0;
                    for (const auto& item : dog->GetBag()) {
                        int loot_type = item.type;
                        if (lootId_to_value_.count(loot_type)) {
                            total_score += lootId_to_value_.at(loot_type);
                        }
                    }
                    dog->AddScore(total_score);
                    dog->ClearBag();
                }
            }
        }
    }

    void GameSession::MoveRemainingPlayers(double delta_time, 
        const std::vector<collision_detector::GatheringEvent>& events) {
        double last_time = 0.0;
        if (!events.empty()) {
            last_time = events.back().time * delta_time;
        }

        double remaining_time = delta_time - last_time;
        for (const auto& [dog_id, dog] : dogs_) {
            MovePlayer(dog_id, remaining_time);
        }
    }
        
    void GameSession::Tick(double delta_time) {
        using namespace collision_detector;

        // 1. Перемещаем игроков
        std::unordered_map<Dog::Id, Pos> new_positions = 
            ComputeNewPositions(delta_time);

        // 2. Определяем события сбора предметов
        std::vector<GatheringEvent> events = 
            DetectGatheringEvents(delta_time);

        // 3. Обрабатываем сбор предметов
        std::unordered_set<size_t> collected_loot_ids = 
            ProcessLootCollection(events, delta_time);

        // 4. Удаляем собранные предметы
        RemoveCollectedLoot(collected_loot_ids);

        // 5. Обрабатываем сдачу лута в офисах
        ProcessLootDelivery();

        // 6. Двигаем игроков на оставшееся время
        MoveRemainingPlayers(delta_time, events);
    }

    void GameSession::RemoveDog(Dog::Id id) {
        if (dogs_.count(id) == 0) return; // Собака уже удалена

        dogs_.erase(id);
        auto it = std::remove_if(dogs_vector_.begin(), dogs_vector_.end(),
                                [id](const auto& dog) { return dog->GetId() == id; });
        dogs_vector_.erase(it, dogs_vector_.end());
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

    Pos GameSession::GenerateRandomRoadPosition() {
        Pos pos;
        int size = regions_.size();
        int random_road_index = GenerateRandomInt(0, size - 1);
        auto road = GetElementByIndex(regions_, random_road_index).second;

        pos.x = GenerateRandomDouble(road.min_x, road.max_x);
        pos.y = GenerateRandomDouble(road.min_y, road.max_y);

        return pos;
    }

    double GameSession::GenerateRandomDouble(double from, double to) {
        std::random_device rd;

        std::mt19937 gen(rd());

        // Определение распределения для чисел с плавающей запятой в интервале [from, to]
        std::uniform_real_distribution<double> dis(from, to);

        return dis(gen);
    }

    int GameSession::GenerateRandomInt(int from, int to) {
        std::random_device rd;

        std::mt19937 gen(rd());

        // Определение распределения для чисел с плавающей запятой в интервале [from, to]
        std::uniform_int_distribution<int> dis(from, to);

        return dis(gen);
    }

    std::shared_ptr<Dog> GameSession::GetDogById(model::Dog::Id id) const {
        if (dogs_.count(id)) {
            return dogs_.at(id);
        }
        
        return nullptr;
    }

    uint64_t GameSession::GetLootCount() {
        return loots_.size();
    }

    void GameSession::GenerateLoot(int count, int loot_types_count) {
        while (loots_.size() < dogs_.size() && count > 0) {
            loots_.push_back(
                LostObject{.id = lost_object_id_++, 
                           .type = static_cast<uint64_t>(rand() % loot_types_count), 
                           .position = GenerateRandomRoadPosition()}
                );
            --count;
        }
    }

    void MapService::AddMap(Map map) {
        const size_t index = common_data_->maps_.size();
        if (auto [it, inserted] = common_data_->map_id_to_index_.emplace(map.GetId(), index); !inserted) {
            throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
        } else {
            try {
               common_data_->maps_.emplace_back(std::move(map));
            } catch (...) {
                common_data_->map_id_to_index_.erase(it);
                throw;
            }
        }
    }

    const MapService::Maps& MapService::GetMaps() const noexcept {
        return common_data_->maps_;
    }

    Game::Game()
        : common_data_(std::make_shared<CommonData>())
        , session_service_(
            std::make_shared<SessionService>(common_data_))
        , map_service_(common_data_)
        , loot_service_(std::make_shared<LootService>(common_data_))
        , engine_(session_service_, loot_service_) {
    }

    void Game::SetDefaultDogSpeed(double default_speed) {
        default_dog_speed_ = default_speed; 
    }

    void Game::SetDefaultTickTime(double delta_time) {
        default_tick_time_ = delta_time;
    }

    void Game::LoadGameData(CommonData data, 
                            loot_gen::LootGeneratorConfig config) { 
        common_data_ = std::make_shared<CommonData>(std::move(data));

        map_service_ = MapService(common_data_);

        session_service_ = std::make_shared<SessionService>((common_data_));
        
        loot_service_ = std::make_shared<LootService>(common_data_);
        loot_service_->ConfigureLootGenerator(config.period, config.probability);
        
        engine_ = GameEngine(session_service_, loot_service_);
    }

    void LootService::ConfigureLootTypes(CommonData::MapLootTypes loot_types) {
        common_data_->mapId_to_lootTypes_ = std::move(loot_types);
    }

    void LootService::ConfigureLootGenerator(double period, double probability) {
        loot_config_.period = period;
        loot_config_.probability = probability;
        auto period_in_ms = 
            std::chrono::milliseconds(static_cast<int>(loot_config_.period * 1000));
        loot_gen_ = loot_gen::LootGenerator(period_in_ms, probability);
    }

    double Game::GetDefaultDogSpeed() const {
        return default_dog_speed_;
    }

    std::shared_ptr<GameSession> SessionService::FindGameSession(Map::Id map_id) {
        // if (!FindMap(map_id)) { return nullptr;}

        if (common_data_->mapId_to_session_index_.count(map_id)) {
            return FindGameSessionBySessionId(common_data_->mapId_to_session_index_[map_id]);
        }

        return CreateGameSession(map_id);
    }

    std::shared_ptr<model::GameSession> 
    SessionService::FindGameSession(model::GameSession::Id session_id) const {
        auto it = common_data_->sessions_[
            common_data_->game_sessions_id_to_index_[session_id]
        ];
        
        return it;
    }

    void SessionService::Tick(std::chrono::milliseconds delta_time) {
        
        for (const auto& session: common_data_->sessions_) {
            session->Tick(static_cast<double>(delta_time.count()) / 1000.0);

            // GenerateLoot(session, delta_time);
        }
    }

    SessionService::SessionService(std::shared_ptr<CommonData> data) 
        : common_data_(data) {    
    }

    void LootService::GenerateLoot(double delta_time) {
        std::chrono::milliseconds interval = 
            std::chrono::milliseconds(static_cast<int>(delta_time));

        for (const auto& session : common_data_->sessions_) {
            unsigned dogs_count = session->GetDogs().size();
            unsigned loot_count = session->GetLootCount();
            int loot_types_count = 
                common_data_->mapId_to_lootTypes_[session->GetMapId()]->size();
            session->GenerateLoot(
                loot_gen_.Generate(interval, loot_count, dogs_count), 
                loot_types_count
            );
        }
    }

    MapService::MapService(std::shared_ptr<CommonData> data) : common_data_(data) {}

    const Map* MapService::FindMap(const Map::Id& id) const noexcept {
        if (auto it = common_data_->map_id_to_index_.find(id); it != common_data_->map_id_to_index_.end()) {
            return &common_data_->maps_.at(it->second);
        }
        
        return nullptr;
    }

    std::shared_ptr<GameSession> SessionService::FindGameSessionBySessionId(GameSession::Id session_id) {
        if (common_data_->game_sessions_id_to_index_.count(session_id)) {
            return common_data_->sessions_[common_data_->game_sessions_id_to_index_[session_id]];
        }

        return nullptr;
    }

}  // namespace model