#pragma once
#include "sdk.h"
#include "tagged.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <optional>
#include <numeric>

const double EPSILON = 1e-9;

namespace model {

    using Dimension = int64_t;
    using Coord = Dimension;
    
    enum class Direction {NORTH, SOUTH, WEST, EAST, DEFAULT};


    struct Pos {
        double x, y;

        bool operator!=(const Pos& other) const {
            return std::abs(x - other.x) > EPSILON || std::abs(y - other.y) > EPSILON;
        }

        Pos operator-(const Pos& other) const {
            return {x - other.x, y - other.y};
        }

        Pos operator+(const Pos& other) const {
            return {x + other.x, y + other.y};
        }

        Pos operator*(double scalar) const {
            return {x * scalar, y * scalar};
        }

        double Dot(const Pos& other) const {
            return x * other.x + y * other.y;
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

        Pos position{0, 0};
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

        Road(HorizontalTag, Point start, Coord end_x) noexcept;

        Road(VerticalTag, Point start, Coord end_y) noexcept;

        bool IsHorizontal() const noexcept;

        bool IsVertical() const noexcept;

        Point GetStart() const noexcept;

        Point GetEnd() const noexcept;

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

        Map(Id id, std::string name) noexcept;

        void SetDefaultDogSpeed(int64_t default_speed);

        double GetDefaultDogSpeed() const;

        const Id& GetId() const noexcept;
        const std::string& GetName() const noexcept;
        const Buildings& GetBuildings() const noexcept;
        const Roads& GetRoads() const noexcept;
        const Offices& GetOffices() const noexcept;

        bool IsDefaultDogSpeedValueConfigured() const;

        void AddRoad(const Road& road);
        void AddBuilding(const Building& building);
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

        Dog(std::string_view name);

        const Id GetId() const;
        const std::string& GetName() const;
        const Pos& GetPosition() const noexcept;
        const Speed& GetSpeed() const noexcept;
        const Direction& GetDirection() const noexcept;
        const State& GetState() const noexcept;


        void SetDefaultDogSpeed(double speed);
        void SetSpeed(double x, double y);
        void SetDogDirSpeed(std::string dir);

        void StopDog();

        Pos MoveDog(Pos new_position);

    private:

        State state_;

        double current_speed = 0;
        double default_dog_speed_ = 0;

        std::string name_;
        static inline Id general_id_ = 0;
    };

    class GameSession {
    public:
        using Id = uint64_t;
        // using DogIdHasher = util::TaggedHasher<Id>;
        using Dogs = std::unordered_map<Dog::Id, std::shared_ptr<Dog>/*, DogIdHasher */>;
    public:
        GameSession(const Map& map);

        Map::Id GetMapId() const;
        double GetMapDefaultSpeed() const;
        const Dogs& GetDogs() const;
        const std::vector<std::string> GetPlayersNames() const;
        const std::vector<State> GetPlayersUnitStates() const;
        Id GetSessionId() const;

        void AddDog(std::shared_ptr<Dog> dog);

        bool HasDog(Dog::Id id);

        void MovePlayer(Dog::Id id, double delta_time);

        void StopPlayer(Dog::Id id);

        void Tick(double delta_time);
    
    private:

        Pos CalculateNewPosition(const Pos& position, const Speed& speed, double delta_time);

        struct Region {
            double min_x, max_x, min_y, max_y;

            bool Contains(const Pos& pos) const {
                return pos.x >= min_x && pos.x <= max_x && pos.y >= min_y && pos.y <= max_y;
            }
        };

        Pos MoveOnMaxDistance(const Pos& current, const Pos& possible, double current_max);

        Pos MaxValueOfRegion(Region reg, Direction dir, Pos current_pos);

        Pos AdjustPositionToMaxRegion(const std::shared_ptr<Dog>& dog);

        void AddRoadToRegions(const Road& road, std::unordered_map<int, Region>& regions);

        void InitializeRegions(const Map& map, std::unordered_map<int, Region>& regions);

        bool IsWithinAnyRegion(const Pos& pos, const std::unordered_map<int, Region>& regions);

        Pos AdjustPositionToRegion(const Pos& position, const Region& region);


        Dogs dogs_;
        const Map& map_;
        // RoadManager road_manager_;
        std::vector<std::shared_ptr<Dog>> dogs_vector_;
        std::unordered_map<int, Region> regions_;

        Id id_;

        static inline Id general_id_{0};
    };

    class Game {
    public:
        using Maps = std::vector<Map>;
        using GameSessions = std::vector<std::shared_ptr<GameSession>>;

        void AddMap(Map map);

        const Maps& GetMaps() const noexcept;
        double GetDefaultDogSpeed() const;

        void SetDefaultDogSpeed(double default_speed);

        std::shared_ptr<GameSession> FindGameSession(Map::Id map_id);

        std::shared_ptr<GameSession> CreateGameSession(Map::Id map_id);

        void Tick(double delta_time);

        const Map* FindMap(const Map::Id& id) const noexcept;

    private:

        double default_dog_speed_ = 1.0;

        std::shared_ptr<GameSession> FindGameSessionBySessionId(GameSession::Id session_id);

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