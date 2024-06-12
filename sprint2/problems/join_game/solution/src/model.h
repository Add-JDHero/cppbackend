#pragma once
#include "sdk.h"
#include "tagged.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <numeric>


namespace model {

    using Dimension = int64_t;
    using Coord = Dimension;
    
    enum class Direction {NORTH, SOUTH, WEST, EAST, DEFAULT};

    struct Point {
        Coord x, y;
    };

    struct Size {
        Dimension width, height;
    };

    struct Speed {
        Dimension x, y;
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

        OfficeIdToIndex warehouse_id_to_index_;
        Offices offices_;
    };

    class Dog {
    public:
        using Id = uint64_t;

        Dog(std::string_view name) : name_(std::string(name)), id_(general_id_++) {};

        const Id GetId() const {
            return id_;
        }
        const std::string& GetName() const {
            return name_;
        }

         const Point& GetPosition() const noexcept {
            return pos_;
        }

        const Speed& GetSpeed() const noexcept {
            return speed_;
        }

        const Direction& GetDirection() const noexcept {
            return direction_;
        }

    private:
        Point pos_;
        Direction dir_;
        Speed speed_{0, 0};
        Direction direction_{Direction::NORTH};

        std::string name_;
        Id id_;

        static inline Id general_id_ = 0;
    };

    class GameSession {
    public:
        using Id = uint64_t;
        // using DogIdHasher = util::TaggedHasher<Id>;
        using Dogs = std::unordered_map<Dog::Id, std::shared_ptr<Dog>/*, DogIdHasher */>;
    public:
        GameSession(const Map& map) : map_(map), id_(++(general_id_)){}

        Map::Id GetMapId() const {
            return map_.GetId();
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

        bool HasDog(Dog::Id id) {
            return dogs_.count(id) > 0;
        }
    
    private:
        Dogs dogs_;
        const Map& map_;

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