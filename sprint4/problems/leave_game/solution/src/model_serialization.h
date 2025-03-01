#pragma once

#include "model.h"
// #include "application.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <boost/json.hpp>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>

namespace model {
    template <typename Archive, typename T, typename Tag>
    void serialize(Archive& ar, util::Tagged<T, Tag>& tagged, const unsigned int version) {
        ar & *tagged;
    }

    template <typename Archive>
    void serialize(Archive& ar, model::Point& point, const unsigned int version) {
        ar & point.x;
        ar & point.y;
    }

    template <typename Archive>
    void serialize(Archive& ar, model::Size& size, const unsigned int version) {
        ar & size.width;
        ar & size.height;
    }

    template <typename Archive>
    void serialize(Archive& ar, model::Rectangle& rect, const unsigned int version) {
        ar & rect.position;
        ar & rect.size;
    }

    template <typename Archive>
    void serialize(Archive& ar, model::Offset& offset, const unsigned int version) {
        ar & offset.dx;
        ar & offset.dy;
    }

    template <typename Archive>
    void serialize(Archive& ar, model::Pos& point, [[maybe_unused]] const unsigned version) {
        ar & point.x;
        ar & point.y;
    }

    template <typename Archive>
    void serialize(Archive& ar, model::Speed& speed, [[maybe_unused]] const unsigned version) {
        ar & speed.x;
        ar & speed.y;
    }

    template <typename Archive>
    void serialize(Archive& ar, model::GameSession::LostObject& obj, [[maybe_unused]] const unsigned version) {
        ar & obj.id;
        ar & obj.type;
        ar & obj.position;
    }

    template <typename Archive>
    void serialize(Archive& ar, model::FoundObject& obj, [[maybe_unused]] const unsigned version) {
        ar & obj.id;
        ar & obj.type;
    }
}

namespace serialization {

    template <typename Archive>
    void serialize(Archive& ar, boost::json::value& json_val, const unsigned int version) {
        std::string json_str;
        
        if constexpr (Archive::is_saving::value) {
            json_str = boost::json::serialize(json_val);
        }

        ar & json_str;

        if constexpr (Archive::is_loading::value) {
            json_val = boost::json::parse(json_str);
        }
    }

    using milliseconds = std::chrono::milliseconds;

    class StateSer {
    public:
        StateSer() = default;
        StateSer(const model::State& state) 
            : position_(state.position)
            , speed_(state.speed)
            , direction_(state.direction)
            , bag_(state.bag)
            , score_(state.score)
            , id_(state.id) { 
        }

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar & position_;
            ar & speed_;
            ar & direction_;
            ar & score_;
            ar & id_;
            ar & bag_;
        }

        [[nodiscard]] model::State Restore() const {
            model::State state;

            state.score = score_;
            state.bag = bag_;
            state.direction = direction_;
            state.id = id_;
            state.position = position_;
            state.speed = speed_;

            return state;
        }

    private:
        model::Pos position_{0, 0};
        model::Speed speed_{0, 0};
        model::Direction direction_{model::Direction::DEFAULT};
        std::vector<model::FoundObject> bag_;
        int score_ = 0;
        model::State::Id id_;
    };

    class DogSer {
    public:
        DogSer() = default;
        DogSer(const model::Dog& dog) 
            : state_(dog.GetState())
            , name_(dog.GetName()) {
        }

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar & name_;
            ar & state_;
        }

        [[nodiscard]] model::Dog Restore() const {
            model::Dog dog(state_.Restore(), name_);

            return dog;
        }

    private:
        StateSer state_;
        std::string name_;
    };

    class RoadSer {
    public:
        RoadSer() = default;
        explicit RoadSer(const model::Road& road) : start_(road.GetStart()), end_(road.GetEnd()) {}

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar & start_;
            ar & end_;
        }

        [[nodiscard]] model::Road Restore() const {
            return model::Road(model::Road::HORIZONTAL, start_, end_.x); // Предполагаем горизонтальность
        }

    private:
        model::Point start_;
        model::Point end_;
    };

    class BuildingSer {
    public:
        BuildingSer() = default;
        explicit BuildingSer(const model::Building& building) : bounds_(building.GetBounds()) {}

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar & bounds_;
        }

        [[nodiscard]] model::Building Restore() const {
            return model::Building(bounds_);
        }

    private:
        model::Rectangle bounds_;
    };

    class OfficeSer {
    public:
        OfficeSer() = default;
        explicit OfficeSer(const model::Office& office)
            : id_(office.GetId()), position_(office.GetPosition()), offset_(office.GetOffset()) {}

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar & id_;
            ar & position_;
            ar & offset_;
        }

        [[nodiscard]] model::Office Restore() const {
            return model::Office(id_, position_, offset_);
        }

    private:
        model::Office::Id id_;
        model::Point position_;
        model::Offset offset_;
    };

    // Сериализация карты
    class MapSer {
    public:
        MapSer() = default;
        explicit MapSer(const model::Map& map)
            : id_(map.GetId()), name_(map.GetName()), bag_capacity_(map.GetBagCapacity()) {
            for (const auto& road : map.GetRoads()) roads_.emplace_back(road);
            for (const auto& building : map.GetBuildings()) buildings_.emplace_back(building);
            for (const auto& office : map.GetOffices()) offices_.emplace_back(office);
        }

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar & id_;
            ar & name_;
            ar & bag_capacity_;
            ar & roads_;
            ar & buildings_;
            ar & offices_;
        }

        [[nodiscard]] model::Map Restore() const {
            model::Map map(id_, name_);
            map.SetBagCapacity(bag_capacity_);
            for (const auto& road : roads_) map.AddRoad(road.Restore());
            for (const auto& building : buildings_) map.AddBuilding(building.Restore());
            for (const auto& office : offices_) map.AddOffice(office.Restore());
            return map;
        }

    private:
        model::Map::Id id_;
        std::string name_;
        int bag_capacity_;
        std::vector<RoadSer> roads_;
        std::vector<BuildingSer> buildings_;
        std::vector<OfficeSer> offices_;
    };

    class GameSessionSer {
    public:
        GameSessionSer() = default;
        GameSessionSer(const model::GameSession& session, model::Map map) 
            : map_(MapSer(map))
            , lootId_to_value_(session.GetLootValuesTable())
            , lost_objects_(session.GetLostObjects()) {
                for (const auto& [dog_id, dog] : session.GetDogs()) {
                    dogs_[dog_id] = std::make_shared<DogSer>(*dog);
                }
        }

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar & dogs_;
            ar & map_;
            ar & lootId_to_value_;
            ar & lost_objects_;
        }

        [[nodiscard]] model::GameSession Restore(const model::CommonData& data) const {
            model::Map restored_map = map_.Restore();
            size_t map_index = data.map_id_to_index_.at(restored_map.GetId());

            model::GameSession session(data.maps_[map_index], lootId_to_value_);

            for (const auto& [id, dog] : dogs_) {
                session.AddDog(std::make_shared<model::Dog>(dog->Restore()));
            }
            return session;
        }

    private:
        std::unordered_map<model::Dog::Id, std::shared_ptr<DogSer>> dogs_;
        MapSer map_;
        model::GameSession::LootIdToValue lootId_to_value_;
        model::GameSession::LostObjects lost_objects_;
    };

    struct CommonDataSer {
        CommonDataSer() = default;
        CommonDataSer(const model::CommonData& data) 
            : game_sessions_id_to_index_(data.game_sessions_id_to_index_)
            , mapId_to_session_index_(data.mapId_to_session_index_)
            , map_id_to_index_(data.map_id_to_index_) {
            for (const auto& map : data.maps_) {
                maps_.push_back(MapSer(map));
            }

            for (const auto& session : data.sessions_) {
                auto index = data.map_id_to_index_.at(session->GetMapId());
                sessions_.emplace_back(*session, data.maps_[index]);
            }

            for (const auto& [map_id, loot_ptr] : data.mapId_to_lootTypes_) {
                if (loot_ptr) {
                    std::vector<std::string> json_strings;
                    for (const auto& val : *loot_ptr) {
                        json_strings.push_back(boost::json::serialize(val));
                    }
                    mapId_to_lootTypes_[map_id] = std::move(json_strings);
                } else {
                    mapId_to_lootTypes_[map_id] = {};
                }
            }

        }

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar & sessions_;
            ar & game_sessions_id_to_index_;
            ar & mapId_to_lootTypes_;  // Теперь это std::vector<boost::json::value>
            ar & mapId_to_session_index_;
            ar & maps_;
            ar & map_id_to_index_;
        }

        [[nodiscard]] model::CommonData Restore() const {
            model::CommonData data;

            for (const auto& map_ser : maps_) {
                data.maps_.emplace_back(map_ser.Restore());
            }
            data.map_id_to_index_ = map_id_to_index_;

            for (const auto& session_ser_ptr : sessions_) {
                auto session = session_ser_ptr.Restore(data);
                data.sessions_.push_back(
                    std::make_shared<model::GameSession>(session));
            }


            data.game_sessions_id_to_index_ = game_sessions_id_to_index_;
            data.mapId_to_lootTypes_ = {};
            for (const auto& [map_id, loot_strings] : mapId_to_lootTypes_) {
                auto json_array = std::make_shared<boost::json::array>();
                for (const auto& str : loot_strings) {
                    json_array->push_back(boost::json::parse(str));
                }
                data.mapId_to_lootTypes_[map_id] = std::move(json_array);
            }
            data.mapId_to_session_index_ = mapId_to_session_index_;

            return data;
        }

    private:
        std::vector<GameSessionSer> sessions_;
        model::CommonData::GameSessionIdToIndex game_sessions_id_to_index_;

        std::unordered_map<model::Map::Id, std::vector<std::string>, util::TaggedHasher<model::Map::Id>> mapId_to_lootTypes_;

        model::CommonData::MapIdToGameSessions mapId_to_session_index_;
        std::vector<MapSer> maps_;
        model::CommonData::MapIdToIndex map_id_to_index_;
    };



    // Сериализация всей игры
    class GameSer {
    public:
        GameSer() = default;
        explicit GameSer(const model::Game& game)
            : serialized_data_(game.GetCommonData())
            , default_dog_speed_(game.GetDefaultDogSpeed())
            , default_tick_time_(game.GetDefaultTickTime()) {
        }

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar & serialized_data_;
            ar & default_dog_speed_;
            ar & default_tick_time_;
        }

        [[nodiscard]] model::Game Restore() const {
            model::Game game;

            game.SetCommonData(std::move(serialized_data_.Restore()));
            game.SetDefaultDogSpeed(default_dog_speed_);
            game.SetDefaultTickTime(default_tick_time_);

            return game;
        }

    private:
        CommonDataSer serialized_data_;

        double default_dog_speed_ = 1.0;
        double default_tick_time_ = 0;
    };

} // namespace serialization

