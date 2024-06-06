#pragma once
#include "sdk.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <numeric>

#include "tagged.h"

namespace model {

    using Dimension = int64_t;
    using Coord = Dimension;
    using Speed = double;

    enum class Direction {NORTH, SOUTH, WEST, EAST};

    struct Point {
        Coord x, y;
    };

    struct Size {
        Dimension width, height;
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

        Dog(std::string name) : name_(name), id_(general_id_++) {};

        const Id GetId() const {
            return id_;
        }
        const std::string& GetName() const {
            return name_;
        }

    private:
        Point pos_;
        Direction dir_;
        Speed speed_;

        std::string name_;
        Id id_;

        static inline Id general_id_ = 0;
    };

    class GameSession {
        // using Id = util::Tagged<uint64_t, Dog>;
        // using DogIdHasher = util::TaggedHasher<Id>;
        using Dogs = std::unordered_map<Dog::Id, std::shared_ptr<Dog>/*, DogIdHasher */>;
    public:
        GameSession(const Map& map) : map_(map){}

        Map::Id GetMapId() const {
            return map_.GetId();
        }

        void AddDog(std::shared_ptr<Dog> dog) {
            dogs_.insert({dog->GetId(), dog});
        }

        const Dogs& GetDogs() const {
            return dogs_;
        }

        bool HasDog(Dog::Id id) {
            return dogs_.count(id) > 0;
        }
    
    private:
        Dogs dogs_;
        const Map& map_;
    };

    class Game {
    public:
        using Maps = std::vector<Map>;
        using GameSessions = std::vector<std::shared_ptr<GameSession>>;

        void AddMap(Map map);

        const Maps& GetMaps() const noexcept {
            return maps_;
        }

        const Map* FindMap(const Map::Id& id) const noexcept {
            if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
                return &maps_.at(it->second);
            }
            
            return nullptr;
        }

    private:
        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

        std::vector<Map> maps_;
        MapIdToIndex map_id_to_index_;
        GameSessions game_sessions_;
    };

}  // namespace model