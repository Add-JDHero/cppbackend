#pragma once
#include "extra_data.h"
#include "tagged.h"
#include "util.h"

// #include "application.h"
#include "collision_detector.h"
#include "loot_generator.h"
#include "sdk.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
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

    struct FoundObject {
        uint64_t id;
        uint64_t type;
    };

    struct State {
        using Id = uint64_t;

        Pos position{0, 0};
        Speed speed{0, 0};
        Direction direction{Direction::DEFAULT};
        std::vector<FoundObject> bag;
        int score = 0;
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
    public:
        struct HorizontalTag {
            explicit HorizontalTag() = default;
        };

        struct VerticalTag {
            explicit VerticalTag() = default;
        };

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

        double GetDefaultDogSpeed() const noexcept;
        const Id& GetId() const noexcept;
        const std::string& GetName() const noexcept;
        const Buildings& GetBuildings() const noexcept;
        const Roads& GetRoads() const noexcept;
        const Offices& GetOffices() const noexcept;
        int GetBagCapacity() const noexcept;

        bool IsDefaultDogSpeedValueConfigured() const noexcept;

        void SetDefaultDogSpeed(int64_t default_speed);
        void SetBagCapacity(int capacity);

        void AddRoad(const Road& road);
        void AddBuilding(const Building& building);
        void AddOffice(Office office);

    private:
        using OfficeIdToIndex = 
            std::unordered_map<Office::Id, 
                               size_t, 
                               util::TaggedHasher<Office::Id>>;

        Id id_;
        std::string name_;
        Roads roads_;
        Buildings buildings_;
        OfficeIdToIndex warehouse_id_to_index_;
        Offices offices_;

        double default_dog_speed_ = 1.0;

        size_t bag_capacity_ = 3;
    };

    class Dog {
    public:
        using Id = uint64_t;

        Dog(State state, std::string name);

        Dog(std::string_view name);

        const Id GetId() const noexcept;
        const std::string& GetName() const noexcept;
        const Pos& GetPosition() const noexcept;
        const Speed& GetSpeed() const noexcept;
        const Direction& GetDirection() const noexcept;
        const State& GetState() const noexcept;
        const double GetDefaultDogSpeed() const noexcept;
        const std::vector<FoundObject>& GetBag() const noexcept;

        void AddToBag(uint64_t loot_id, uint64_t loot_type);
        void AddScore(int score);

        void ClearBag();

        void SetDefaultDogSpeed(double speed);
        void SetSpeed(double x, double y);
        void SetDogDirSpeed(std::string dir);
        void SetRandomPosition(Pos pos);

        void StopDog();

        Pos MoveDog(Pos new_position);

    private:
        void SetBagCapacity(size_t capacity);

        State state_;

        // double current_speed = 0;
        double default_dog_speed_ = 0;

        size_t bag_capacity_ = 3;

        std::string name_;
        static inline Id general_id_ = 0;
    };

    class GameSession {
    public:
        struct LostObject {
            uint64_t id;
            uint64_t type;
            model::Pos position;
        };
        
        struct Region {
            double min_x, max_x, min_y, max_y;

            bool Contains(const Pos& pos) const {
                return pos.x >= min_x && 
                       pos.x <= max_x && 
                       pos.y >= min_y && 
                       pos.y <= max_y;
            }
        };

        using Id = uint64_t;
        using Dogs = std::unordered_map<Dog::Id, std::shared_ptr<Dog>>;
        using LostObjects = std::vector<LostObject>;
        using LootTypeToValue = std::unordered_map<int, int>;
    public:
        GameSession(const Map& map, 
                    std::unordered_map<int, int> loot_values,  
                    bool is_deserialized = false);

        Map::Id GetMapId() const noexcept;
        Id GetSessionId() const noexcept;
        double GetMapDefaultSpeed() const noexcept;
        const Dogs& GetDogs() const noexcept;
        const std::vector<std::shared_ptr<Dog>> GetDogsVector() const noexcept { return dogs_vector_; }
        // const std::unordered_map<int, Region> GetRegions() const { return regions_; }
        const std::vector<std::string> GetPlayersNames() const noexcept;
        const std::vector<State> GetPlayersUnitStates() const noexcept;
        const LostObjects GetLostObjects() const noexcept { return loots_; }
        const LootTypeToValue GetLootValuesTable() const noexcept { return lootId_to_value_; }
        uint64_t GetLostObjectsVal() const noexcept { return lost_object_id_; };
        std::shared_ptr<Dog> GetDogById(model::Dog::Id id) const;

        void SetDeserializedSessionId(Id id) { id_ = id; }
        void SetDeserializedObjVal(uint64_t lost_obect_id) { lost_object_id_ = lost_obect_id; }

        int GetLootValue(int loot_type) const;


        uint64_t GetLootCount();

        void GenerateLoot(int count, int loot_types_count);

        Pos GenerateRandomRoadPosition();

        void LootRecovery(LostObjects loot) { loots_ = std::move(loot); }

        std::vector<collision_detector::Gatherer>
        GetGatherers(double delta_time) const;

        std::vector<collision_detector::Item>
        GetItems() const;

        bool HasDog(Dog::Id id);

        void AddDog(std::shared_ptr<Dog> dog, bool is_deserialized = false);

        void MovePlayer(Dog::Id id, double delta_time);

        void StopPlayer(Dog::Id id);

        void Tick(double delta_time);

        void RemoveDog(Dog::Id id);
    
    private:
        Pos CalculateNewPosition(const Pos& position, const Speed& speed, double delta_time);

        std::unordered_map<Dog::Id, Pos> 
        ComputeNewPositions(double delta_time);

        std::vector<collision_detector::GatheringEvent> 
        DetectGatheringEvents(double delta_time);

        std::unordered_set<size_t> ProcessLootCollection();

        std::unordered_set<size_t> 
        ProcessLootCollection(const std::vector<collision_detector::GatheringEvent>& events, 
                              double delta_time);

        void ProcessLootDelivery();

        void MoveRemainingPlayers(double delta_time, 
                                  const std::vector<collision_detector::GatheringEvent>& events);
        
        void RemoveCollectedLoot(const std::unordered_set<size_t>& collected_loot_ids);

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

        LootTypeToValue lootId_to_value_;
        LostObjects loots_;
        Id id_;

        size_t bag_capacity_;

        static inline Id general_id_{0};
        static inline uint64_t lost_object_id_{0};
    };

    struct CommonData {
        using Maps = std::vector<Map>;
        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = 
            std::unordered_map<Map::Id, size_t, MapIdHasher>;

        using MapIdToGameSessions =
            std::unordered_map<Map::Id, GameSession::Id, MapIdHasher>;

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
        MapService(std::shared_ptr<CommonData> data);

        void AddMap(model::Map map);
        
        const model::Map* FindMap(const model::Map::Id& id) const noexcept;

        const Maps& GetMaps() const noexcept;

    private:
        std::shared_ptr<CommonData> common_data_;
    };


    class SessionService {
    public:
        using GameSessions = std::vector<std::shared_ptr<GameSession>>;

        SessionService(std::shared_ptr<CommonData> data);

        std::shared_ptr<model::GameSession> 
        CreateGameSession(model::Map::Id map_id);
        
        std::shared_ptr<model::GameSession> 
        FindGameSession(model::Map::Id map_id);

        std::shared_ptr<model::GameSession> 
        FindGameSession(model::GameSession::Id session_id) const;

        std::shared_ptr<GameSession> 
        FindGameSessionBySessionId(GameSession::Id session_id);

        void Tick(std::chrono::milliseconds delta_time);

    private:
        void ConfigureSessionData(std::shared_ptr<GameSession> session);

        std::shared_ptr<CommonData> common_data_;
    };

    class LootService {
    public:
        explicit LootService(std::shared_ptr<CommonData> data) 
            : common_data_(data) {    
        }

        void GenerateLoot(double delta_time);

        void ConfigureLootTypes(CommonData::MapLootTypes loot_types);

        void ConfigureLootGenerator(double period, double probability);

        loot_gen::LootGeneratorConfig GetGeneratorConfig() const { return loot_config_; }

        const CommonData::MapLootTypes& GetLootTypes() {
            return common_data_->mapId_to_lootTypes_;
        }

    private:
        std::shared_ptr<CommonData> common_data_;

        loot_gen::LootGeneratorConfig loot_config_;
        loot_gen::LootGenerator loot_gen_{0ms, 0.5};
    };

    class GameEngine {
    public:
        GameEngine(std::shared_ptr<SessionService> session_service, 
                   std::shared_ptr<LootService> loot_service)
            : session_service_(session_service)
            , loot_service_(loot_service) {
        }

        void Tick(std::chrono::milliseconds delta_time) {
            session_service_->Tick(delta_time);
            loot_service_->GenerateLoot(delta_time.count());
        }

    private:
        std::shared_ptr<SessionService> session_service_;
        std::shared_ptr<LootService> loot_service_;
    };

    class Game {
    public:
        explicit Game();
 
        void LoadGameData(CommonData data, loot_gen::LootGeneratorConfig config);

        loot_gen::LootGeneratorConfig 
        GetGeneratorConfig() const { return loot_service_->GetGeneratorConfig(); }

        double GetDefaultDogSpeed() const;
        double GetDefaultTickTime() const { return default_tick_time_; }
        CommonData& GetCommonData() const { return *common_data_; }

        GameEngine& GetEngine() { return engine_; }
        SessionService& GetSessionService() { return *session_service_; }
        MapService& GetMapService() { return map_service_; }
        LootService& GetLootService() { return *loot_service_; }

        void SetDefaultTickTime(double delta_time);
        void SetDefaultDogSpeed(double default_speed);
    private:
        double default_dog_speed_ = 1.0;
        double default_tick_time_ = 0;

        std::shared_ptr<CommonData> common_data_;

        MapService map_service_;
        
        std::shared_ptr<SessionService> session_service_;
        std::shared_ptr<LootService> loot_service_;
        GameEngine engine_;
    };

}  // namespace model