#pragma once

#include "sdk.h"
#include "model.h"
#include "json_loader.h"
#include "extra_data.h"

#include <filesystem>
#include <boost/json.hpp>

namespace json_loader {
    namespace json = boost::json;
    using namespace std::literals;
    
    /* model::CommonData::MapLootTypes
    ExtractLootTypes(const MapLootTypes& loot_types); */

    model::CommonData::MapLootTypes ParseLootTypes(const json::value& obj);

    loot_gen::LootGeneratorConfig ParseLootGeneratorConfig(const json::object& obj);

    class MapParser {
    public:
        static std::vector<model::Map> Parse(const json::value& jsonVal);

    private:
        static model::Map ParseSingleMap(const json::object& obj);
        
        static std::vector<model::Road> ParseRoads(const json::array& roadsArray);
        static std::vector<model::Building> ParseBuildings(const json::array& buildingsArray);
        static std::vector<model::Office> ParseOffices(const json::array& officesArray);
    };

    class MapSerializer {
    public:
        static std::string SerializeMaps(const std::vector<model::Map>& maps);
        static std::string SerializeMapsMainInfo(const std::vector<model::Map>& maps);
        static json::object SerializeSingleMap(const model::Map& map);
        static json::object SerializeSingleMapMainInfo(const model::Map& map);

    private:
        static json::array SerializeRoads(const std::vector<model::Road>& roads);
        static json::array SerializeBuildings(const std::vector<model::Building>& buildings);
        static json::array SerializeOffices(const std::vector<model::Office>& offices);
    };

    class StateSerializer {
    public:
        static std::string SerializeStates(const std::vector<model::State>& states,
                                           const model::GameSession::LostObjects& lost_objects);
        static json::object SerializeSingleState(const model::State& state);
        static json::object SerializeSingleLostObject(const model::GameSession::LostObject lost_object);

    private:
        static json::array SerializePoint(const model::Pos& point);
        static json::array SerializeSpeed(const model::Speed& speed);
        static std::string SerializeDirection(model::Direction direction);
    };

    json::value ParseConfigFile(std::string s);

    model::Game LoadGame(const std::filesystem::path& file_path);

}  // namespace json_loader