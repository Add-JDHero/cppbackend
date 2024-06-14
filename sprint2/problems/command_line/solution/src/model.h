#pragma once
#include "sdk.h"
#include "tagged.h"

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <optional>
#include <numeric>

const double EPSILON = 1e-9;

namespace model {

    class Dog;
    class Road;

    namespace bg = boost::geometry;
    namespace bgi = boost::geometry::index;

    using point = bg::model::d2::point_xy<double>;
    using box = bg::model::box<point>;
    using road_value = std::pair<box, std::shared_ptr<Road>>;

    using Dimension = int64_t;
    using Coord = Dimension;
    
    enum class Direction {NORTH, SOUTH, WEST, EAST, DEFAULT};


    struct Pos {
        double x, y;

        bool operator!=(const Pos& other) const {
            return std::abs(x - other.x) > EPSILON || std::abs(y - other.y) > EPSILON;
        }
    };

    struct Point {
        Dimension x, y;
    };

    struct Size {
        Dimension width, height;
    };

    struct Speed {
        double x, y;
    };

    struct State {
        using Id = uint64_t;

        Pos position{0, 10};
        Speed speed{0, 0};
        Direction direction{Direction::DEFAULT};
        Id id;
    };

    struct Rectangle {
        Point position;
        Size size;
    };

    struct Offset {
        Dimension dx, dy;
    };

    class Road {
        struct HorizontalTag {
            explicit HorizontalTag() = default;
        };

        struct VerticalTag {
            explicit VerticalTag() = default;
        };

    public:
        constexpr static HorizontalTag HORIZONTAL{};
        constexpr static VerticalTag VERTICAL{};

        Road(HorizontalTag, Point start, Coord end_x) noexcept
            : start_{start}
            , end_{end_x, start.y} {
        }

        Road(VerticalTag, Point start, Coord end_y) noexcept
            : start_{start}
            , end_{start.x, end_y} {
        }

        bool IsHorizontal() const noexcept {
            return start_.y == end_.y;
        }

        bool IsVertical() const noexcept {
            return start_.x == end_.x;
        }

        Point GetStart() const noexcept {
            return start_;
        }

        Point GetEnd() const noexcept {
            return end_;
        }

    private:
        Point start_;
        Point end_;
    };

    class Building {
    public:
        explicit Building(const Rectangle& bounds) noexcept
            : bounds_{bounds} {
        }

        const Rectangle& GetBounds() const noexcept {
            return bounds_;
        }

    private:
        Rectangle bounds_;
    };

    class Office {
    public:
        using Id = util::Tagged<std::string, Office>;

        Office(Id id, Point position, Offset offset) noexcept
            : id_{std::move(id)}
            , position_{position}
            , offset_{offset} {
        }

        const Id& GetId() const noexcept {
            return id_;
        }

        Point GetPosition() const noexcept {
            return position_;
        }

        Offset GetOffset() const noexcept {
            return offset_;
        }

    private:
        Id id_;
        Point position_;
        Offset offset_;
    };

    class Map {
    public:
        using Id = util::Tagged<std::string, Map>;
        using Roads = std::vector<Road>;
        using Buildings = std::vector<Building>;
        using Offices = std::vector<Office>;

        Map(Id id, std::string name) noexcept
            : id_(std::move(id))
            , name_(std::move(name)) {
        }

        const Id& GetId() const noexcept {
            return id_;
        }

        void SetDefaultDogSpeed(int64_t default_speed) {
            default_dog_speed_ = default_speed; 
        }
        
        double GetDefaultDogSpeed() const {
            return default_dog_speed_;
        }

        bool IsDefaultDogSpeedValueConfigured() const {
            return default_dog_speed_ != 1;
        }

        const std::string& GetName() const noexcept {
            return name_;
        }

        const Buildings& GetBuildings() const noexcept {
            return buildings_;
        }

        const Roads& GetRoads() const noexcept {
            return roads_;
        }

        const Offices& GetOffices() const noexcept {
            return offices_;
        }

        void AddRoad(const Road& road) {
            roads_.emplace_back(road);
        }

        void AddBuilding(const Building& building) {
            buildings_.emplace_back(building);
        }

        void AddOffice(Office office);

    private:
        using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

        Id id_;
        std::string name_;
        Roads roads_;
        Buildings buildings_;

        double default_dog_speed_ = 1.0;

        OfficeIdToIndex warehouse_id_to_index_;
        Offices offices_;
    };

    class Dog {
    public:
        using Id = uint64_t;

        Dog(std::string_view name) : name_(std::string(name)) {
            state_.id = general_id_++;
        };

        const Id GetId() const {
            return state_.id;
        }
        const std::string& GetName() const {
            return name_;
        }

         const Pos& GetPosition() const noexcept {
            return state_.position;
        }

        const Speed& GetSpeed() const noexcept {
            return state_.speed;
        }

        const Direction& GetDirection() const noexcept {
            return state_.direction;
        }

        void SetSpeed(double x, double y) {
            state_.speed.x = x;
            state_.speed.y = y;
        }
        
        Pos MoveDog(Pos new_position) {
            state_.position = new_position;
            return state_.position;
        }

        void SetDefaultDogSpeed(double speed) {
            default_dog_speed_ = speed;
        }

        void StopDog() {
            state_.speed = {0, 0};
        }

        void SetDogSpeed(std::string dir) {
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
            }

            return;
            
        }

        const State& GetState() const noexcept {
            return state_;
        }

    private:

        State state_;

        double current_speed = 0;
        double default_dog_speed_ = 0;

        std::string name_;
        static inline Id general_id_ = 0;
    };

    /* class RoadManager {
    public:
        RoadManager(const std::vector<Road>& roads) {
            for (const auto& road : roads) {
                auto road_box = CreateRoadBoundingBox(road);
                rtree_.insert(std::make_pair(road_box, std::make_shared<Road>(road)));
            }
        }

        std::vector<std::shared_ptr<Road>> GetIntersectingRoads(const point& p) const {
            std::vector<road_value> result;
            rtree_.query(bgi::intersects(p), std::back_inserter(result));
            std::vector<std::shared_ptr<Road>> roads;
            for (const auto& value : result) {
                roads.push_back(value.second);
            }
            return roads;
        }

    private:
        bgi::rtree<road_value, bgi::quadratic<16>> rtree_;

        box CreateRoadBoundingBox(const Road& road) const {
            if (road.IsHorizontal()) {
                return box(point(road.GetStart().x, road.GetStart().y - 0.4),
                        point(road.GetEnd().x, road.GetStart().y + 0.4));
            } else {
                return box(point(road.GetStart().x - 0.4, road.GetStart().y),
                        point(road.GetStart().x + 0.4, road.GetEnd().y));
            }
        }
    }; */


    class GameSession {
    public:
        using Id = uint64_t;
        // using DogIdHasher = util::TaggedHasher<Id>;
        using Dogs = std::unordered_map<Dog::Id, std::shared_ptr<Dog>/*, DogIdHasher */>;
    public:
        GameSession(const Map& map) : map_(map), /* road_manager_(map.GetRoads()), */ id_(general_id_++) {
            InitializeRegions(map_, regions_);
        }

        Map::Id GetMapId() const {
            return map_.GetId();
        }

        double GetMapDefaultSpeed() const {
            return map_.GetDefaultDogSpeed();
        }

        Id GetSessionId() const {
            return general_id_;
        }

        void AddDog(std::shared_ptr<Dog> dog) {
            dogs_.insert({dog->GetId(), dog});
        }

        const Dogs& GetDogs() const {
            return dogs_;
        }

        const std::vector<std::string> GetPlayersNames() const {
            std::vector<std::string> names;
            for (const auto& [dog_id, dog]: dogs_) {
                names.push_back(dog->GetName());
            }

            return names;
        }

        const std::vector<State> GetPlayersUnitStates() const {
            std::vector<State> dogs;
            for (const auto& [dog_id, dog]: dogs_) {
                dogs.push_back(dog->GetState());
            }

            return dogs;
        }

        bool HasDog(Dog::Id id) {
            return dogs_.count(id) > 0;
        }

        /* void MovePlayer(Dog::Id id, double delta_time) {
            auto& dog = dogs_[id];
            auto new_position = CalculateNewPosition(dog->GetPosition(), dog->GetSpeed(), delta_time);
            auto adjusted_position = AdjustPositionToRoads(new_position);

            if (adjusted_position != new_position) {
                dog->StopDog();
            }

            dog->MoveDog(adjusted_position);
       } */

        void MovePlayer(Dog::Id id, double delta_time) {
            auto& dog = dogs_[id];
            auto new_position = CalculateNewPosition(dog->GetPosition(), dog->GetSpeed(), delta_time);

            if (IsWithinAnyRegion(new_position, regions_)) {
                dog->MoveDog(new_position);
            } else {
                for (const auto& [id, region] : regions_) {
                    if (region.Contains(dog->GetPosition())) {
                        auto adjusted_position = AdjustPositionToRegion(new_position, region);
                        if (adjusted_position != new_position) {
                            dog->StopDog();
                        }
                        dog->MoveDog(adjusted_position);
                        break;
                    }
                }
            }
        }

        void StopPlayer(Dog::Id id) {
            dogs_[id]->StopDog();
        }

        void Tick(double delta_time) {
            for (const auto& [dog_id, dog]: dogs_) {
                MovePlayer(dog_id, delta_time);
            }
        }
    
    private:

        Pos CalculateNewPosition(const Pos& position, const Speed& speed, double delta_time) {
            Pos result{};

            result.x = position.x + speed.x * delta_time;
            result.y = position.y + speed.y * delta_time;

            return result;
        }

        /* bool AreDoublesEqual(double a, double b) {
            return std::abs(a - b) < EPSILON;
        }

        Pos AdjustHorisontalRoad(const Pos& position, const Road& road) {
            Pos adjusted = position;
            double min_y = static_cast<double>(road.GetStart().y * 1.0 - 0.4);
            double max_y = static_cast<double>(road.GetStart().y * 1.0 + 0.4);
            double min_x = static_cast<double>(road.GetStart().x);
            double max_x = static_cast<double>(road.GetEnd().x);

            if (position.x > max_x) { adjusted.x = max_x; }
            if (position.x < min_x) { adjusted.x = min_x; }

            if (position.y > max_y) { adjusted.y = max_y; }
            if (position.y < min_y) { adjusted.y = min_y; }

            return adjusted;

        }

        Pos AdjustVerticalRoad(const Pos& position, const Road& road) {
            Pos adjusted = position;
            double min_x = static_cast<double>(road.GetStart().x * 1.0 - 0.4);
            double max_x = static_cast<double>(road.GetStart().x * 1.0 + 0.4);
            double min_y = static_cast<double>(road.GetStart().y);
            double max_y = static_cast<double>(road.GetEnd().y);

            if (position.x > max_x) { adjusted.x = max_x; }
            if (position.x < min_x) { adjusted.x = min_x; }

            if (position.y > max_y) { adjusted.y = max_y; }
            if (position.y < min_y) { adjusted.y = min_y; }

            return adjusted;
        }

        Pos AdjustPositionForRoad(const Pos& position, const Road& road) {
            Pos adjusted_position = position;
            if (road.IsHorizontal()) {
                adjusted_position = AdjustHorisontalRoad(adjusted_position, road);
            } else if (road.IsVertical()) {
                adjusted_position = AdjustVerticalRoad(adjusted_position, road);
            }
            return adjusted_position;
        } */

         /* Pos AdjustPositionToRoads(const Pos& position) {
            Pos adjusted_position = position;
            point p(position.x, position.y);
            // auto roads = road_manager_.GetIntersectingRoads(p);

            for (const auto& road : roads) {
                adjusted_position = AdjustPositionForRoad(adjusted_position, *road);
            }

            return adjusted_position;
        } */

        struct Region {
            double min_x, max_x, min_y, max_y;

            bool Contains(const Pos& pos) const {
                return pos.x >= min_x && pos.x <= max_x && pos.y >= min_y && pos.y <= max_y;
            }
        };

        // Добавление дорог в регионы
        void AddRoadToRegions(const Road& road, std::unordered_map<int, Region>& regions) {
            if (road.IsHorizontal()) {
                regions.emplace(regions.size(), Region{road.GetStart().x * 1.0, road.GetEnd().x * 1.0, road.GetStart().y - 0.4, road.GetStart().y + 0.4});
            } else if (road.IsVertical()) {
                regions.emplace(regions.size(), Region{road.GetStart().x - 0.4, road.GetStart().x + 0.4, road.GetStart().y * 1.0, road.GetEnd().y * 1.0});
            }
        }

        // Инициализация регионов
        void InitializeRegions(const Map& map, std::unordered_map<int, Region>& regions) {
            for (const auto& road : map.GetRoads()) {
                AddRoadToRegions(road, regions);
            }
        }

        // Проверка, находится ли позиция в любой области
        bool IsWithinAnyRegion(const Pos& pos, const std::unordered_map<int, Region>& regions) {
            for (const auto& [id, region] : regions) {
                if (region.Contains(pos)) {
                    return true;
                }
            }
            return false;
        }

        // Корректировка позиции в рамках региона
        Pos AdjustPositionToRegion(const Pos& position, const Region& region) {
            Pos adjusted = position;
            if (position.x > region.max_x) { adjusted.x = region.max_x; }
            if (position.x < region.min_x) { adjusted.x = region.min_x; }
            if (position.y > region.max_y) { adjusted.y = region.max_y; }
            if (position.y < region.min_y) { adjusted.y = region.min_y; }
            return adjusted;
        }


        Dogs dogs_;
        const Map& map_;
        // RoadManager road_manager_;
        std::unordered_map<int, Region> regions_;

        Id id_;

        static inline Id general_id_{0};
    };

    class Game {
    public:
        using Maps = std::vector<Map>;
        using GameSessions = std::vector<std::shared_ptr<GameSession>>;

        void AddMap(Map map);

        const Maps& GetMaps() const noexcept {
            return maps_;
        }

        void SetDefaultDogSpeed(double default_speed) {
            default_dog_speed_ = default_speed; 
        }

        double GetDefaultDogSpeed() const {
            return default_dog_speed_;
        }

        std::shared_ptr<GameSession> FindGameSession(Map::Id map_id) {
            if (!FindMap(map_id)) { return nullptr;}

            if (map_id_to_session_index_.count(map_id)) {
                return FindGameSessionBySessionId(map_id_to_session_index_[map_id]);
            }

            return CreateGameSession(map_id);
        }

        std::shared_ptr<GameSession> CreateGameSession(Map::Id map_id) {
            auto result = std::make_shared<GameSession>(maps_[map_id_to_index_[map_id]]);
            int index = sessions_.size();
            sessions_.push_back(result);
            game_sessions_id_to_index_[result->GetSessionId()] = index;
            map_id_to_session_index_[map_id] = result->GetSessionId();

            return result;
        }

        void Tick(double delta_time) {
            for (const auto& session: sessions_) {
                session->Tick(delta_time);
            }
        }

        /* 
        std::shared_ptr<GameSession> ConnectToSession(Map::Id map_id, std::string& user_name) {
            std::shared_ptr<GameSession> result = FindGameSession(map_id);

            if (result == nullptr) {
                result = CreateGameSession(map_id);
            }



            return result;
        } */

        const Map* FindMap(const Map::Id& id) const noexcept {
            if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
                return &maps_.at(it->second);
            }
            
            return nullptr;
        }

    private:

        double default_dog_speed_ = 1.0;

        std::shared_ptr<GameSession> FindGameSessionBySessionId(GameSession::Id session_id) {
            if (game_sessions_id_to_index_.count(session_id)) {
                return sessions_[game_sessions_id_to_index_[session_id]];
            }

            return nullptr;
        }

        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

        using GameSessionIdToIndex = std::unordered_map<GameSession::Id, size_t>;
        using MapIdToGameSessions = std::unordered_map<Map::Id, GameSession::Id, MapIdHasher>;


        std::vector<Map> maps_;
        MapIdToIndex map_id_to_index_;
        MapIdToGameSessions map_id_to_session_index_;

        GameSessions sessions_;
        GameSessionIdToIndex game_sessions_id_to_index_;
    };

}  // namespace model