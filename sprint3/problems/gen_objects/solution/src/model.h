#pragma once
#include "tagged.h"
#include "util.h"

// #include "application.h"
#include "loot_generator.h"
#include "sdk.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>
#include <random>
#include <vector>
#include <chrono>
#include <optional>
#include <numeric>

const double EPSILON = 1e-9;

using namespace std::chrono_literals;

namespace model {

    template<typename KeyType, typename ValueType>
    std::pair<KeyType, ValueType> 
    GetElementByIndex(const std::unordered_map<KeyType, ValueType>& map, size_t index) {
        if (index >= map.size()) {
            throw std::out_of_range("Index is out of range");
        }

        auto it = map.begin();
        std::advance(it, index);

        return *it;
    }

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
        using Dogs = std::unordered_map<Dog::Id, std::shared_ptr<Dog>>;
        using LostObjects = std::vector<std::pair<int, Pos>>;
    public:
        GameSession(const Map& map);

        Map::Id GetMapId() const;
        Id GetSessionId() const;
        double GetMapDefaultSpeed() const;
        const Dogs& GetDogs() const;
        const std::vector<std::string> GetPlayersNames() const;
        const std::vector<State> GetPlayersUnitStates() const;
        const LostObjects& GetLostObjects() const {
            return loots_;
        }

        uint64_t GetLootCount();
        void GenerateLoot(int count, int loot_types_count);

        Pos GenerateRandomRoadPosition();

        void AddDog(std::shared_ptr<Dog> dog);

        bool HasDog(Dog::Id id);

        void MovePlayer(Dog::Id id, double delta_time);

        void StopPlayer(Dog::Id id);

        void Tick(double delta_time);

        /* static void SetDefaultTickTime(double delta_time) {
            default_delta_time_ = delta_time;
        } */
    
    private:

        Pos CalculateNewPosition(const Pos& position, const Speed& speed, double delta_time);

        struct Region {
            double min_x, max_x, min_y, max_y;

            bool Contains(const Pos& pos) const {
                return pos.x >= min_x && pos.x <= max_x && pos.y >= min_y && pos.y <= max_y;
            }
        };

        double GenerateRandomDouble(double from, double to);

        int GenerateRandomInt(int from, int to);

        Pos MoveOnMaxDistance(const Pos& current, const Pos& possible, double current_max);

        Pos MaxValueOfRegion(Region reg, Direction dir, Pos current_pos);

        Pos AdjustPositionToMaxRegion(const std::shared_ptr<Dog>& dog);

        void AddRoadToRegions(const Road& road, std::unordered_map<int, Region>& regions);

        void InitializeRegions(const Map& map, std::unordered_map<int, Region>& regions);

        bool IsWithinAnyRegion(const Pos& pos, const std::unordered_map<int, Region>& regions);

        Pos AdjustPositionToRegion(const Pos& position, const Region& region);

        Dogs dogs_;
        const Map& map_;
        std::vector<std::shared_ptr<Dog>> dogs_vector_;
        std::unordered_map<int, Region> regions_;
        std::vector<std::pair<int, Pos>> loots_;
        Id id_;

        static inline Id general_id_{0};

        // static double default_delta_time_;
    };

    struct CommonData {
        using Maps = std::vector<Map>;
        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
        using MapIdToGameSessions = 
            std::unordered_map<Map::Id, GameSession::Id, MapIdHasher>;
        // using MapIdToLootTypesCount = std::unordered_map<Map::Id, int, util::TaggedHasher<Map::Id>>;
        using MapLootTypes =
            std::unordered_map<model::Map::Id, 
                              std::shared_ptr<boost::json::array>,
                              util::TaggedHasher<model::Map::Id>>;
        
        using GameSessions = std::vector<std::shared_ptr<GameSession>>;
        using GameSessionIdToIndex = 
            std::unordered_map<GameSession::Id, size_t>;

        
        GameSessions sessions_;
        GameSessionIdToIndex game_sessions_id_to_index_;

        MapLootTypes mapId_to_lootTypes_;
        MapIdToGameSessions mapId_to_session_index_;

        Maps maps_;
        MapIdToIndex map_id_to_index_;
    };

    class MapService {
    public:
        using Maps = std::vector<Map>;

        MapService() = delete;
        MapService(CommonData& data);

        void AddMap(model::Map map);
        
        const model::Map* FindMap(const model::Map::Id& id) const noexcept;
        const Maps& GetMaps() const noexcept;

    private:
        CommonData& common_data_;
    };


    class SessionService {
    public:
        using GameSessions = std::vector<std::shared_ptr<GameSession>>;

        SessionService(CommonData& data);

        std::shared_ptr<model::GameSession> 
        CreateGameSession(model::Map::Id map_id);
        
        std::shared_ptr<model::GameSession> 
        FindGameSession(model::Map::Id map_id);

        std::shared_ptr<GameSession> 
        FindGameSessionBySessionId(GameSession::Id session_id);

        void Tick(double delta_time);

    private:
        CommonData& common_data_;
    };

    class LootService {
    public:
        explicit LootService(CommonData& data) : common_data_(data) {}

        void GenerateLoot(double delta_time);

        void ConfigureLootTypes(CommonData::MapLootTypes loot_types);

        void ConfigureLootGenerator(double period, double probability);

        const CommonData::MapLootTypes& GetLootTypes() {
            return common_data_.mapId_to_lootTypes_;
        }

    private:
        CommonData& common_data_;

        loot_gen::LootGeneratorConfig loot_config_;
        loot_gen::LootGenerator loot_gen_{0ms, 0};
    };

    class GameEngine {
    public:
        GameEngine(SessionService& session_service, LootService& loot_service)
            : session_service_(session_service)
            , loot_service_(loot_service) {
        }

        void Tick(double delta_time) {
            session_service_.Tick(delta_time);
            loot_service_.GenerateLoot(delta_time);
        }

    private:
        SessionService& session_service_;
        LootService& loot_service_;
    };

    class Game {
    public:
        Game()
            : common_data_()
            , session_service_(common_data_)
            , map_service_(common_data_)
            , loot_service_(common_data_)
            , engine_(session_service_, loot_service_) {
        }

        double GetDefaultDogSpeed() const;

        void SetDefaultTickTime(double delta_time);
        void SetDefaultDogSpeed(double default_speed);

        GameEngine& GetEngine() { return engine_; }
        SessionService& GetSessionService() { return session_service_; }
        MapService& GetMapService() { return map_service_; }
        LootService& GetLootService() { return loot_service_; }

    private:
        CommonData common_data_;

        double default_dog_speed_ = 1.0;
        double default_tick_time_ = 0;

        GameEngine engine_;

        MapService map_service_;
        SessionService session_service_;
        LootService loot_service_;
    };

}  // namespace model