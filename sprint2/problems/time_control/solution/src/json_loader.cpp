#include "json_loader.h"
#include "util.h"

#include <boost/json/array.hpp>
#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>

namespace json_loader {

    namespace json_keys {
        constexpr const char* ID = "id";
        constexpr const char* NAME = "name";
        constexpr const char* MAPS = "maps";
        constexpr const char* ROADS = "roads";
        constexpr const char* BUILDINGS = "buildings";
        constexpr const char* OFFICES = "offices";
        constexpr const char* X = "x";
        constexpr const char* Y = "y";
        constexpr const char* X0 = "x0";
        constexpr const char* Y0 = "y0";
        constexpr const char* X1 = "x1";
        constexpr const char* Y1 = "y1";
        constexpr const char* OFFSET_X = "offsetX";
        constexpr const char* OFFSET_Y = "offsetY";
        constexpr const char* WIDTH = "w";
        constexpr const char* HEIGHT = "h";
        constexpr const char* POS = "pos";
        constexpr const char* SPEED = "speed";
        constexpr const char* DIR = "dir";
        constexpr const char* CONFIG_DEFAULT_SPEED = "defaultDogSpeed";
        constexpr const char* MAP_DEFAULT_SPEED = "dogSpeed";
    }

    namespace json = boost::json;
    using namespace std::literals;

    std::vector<model::Map> MapParser::Parse(const json::value& jsonVal) {
        std::vector<model::Map> maps;
        auto mapsArray = jsonVal.at(json_keys::MAPS).as_array();
        maps.reserve(mapsArray.size());

        std::transform(mapsArray.cbegin(), mapsArray.cend(), std::back_inserter(maps),
            [](const json::value& mapVal) -> model::Map {
                return ParseSingleMap(mapVal.as_object());
            }
        );

        return maps;
    }

    model::Map MapParser::ParseSingleMap(const json::object& obj) {
        model::Map::Id mapId{obj.at(json_keys::ID).as_string().c_str()};
        std::string name = obj.at(json_keys::NAME).as_string().c_str();
        model::Map map(std::move(mapId), std::move(name));

        if (auto val = obj.if_contains(json_keys::MAP_DEFAULT_SPEED)) {
            map.SetDefaultDogSpeed((*val).as_double());
        }

        if (obj.contains(json_keys::ROADS)) {
            auto roads = ParseRoads(obj.at(json_keys::ROADS).as_array());
            for (const auto& road : roads) {
                map.AddRoad(road);
            }
        }

        if (obj.contains(json_keys::BUILDINGS)) {
            auto buildings = ParseBuildings(obj.at(json_keys::BUILDINGS).as_array());
            for (const auto& building : buildings) {
                map.AddBuilding(building);
            }
        }

        if (obj.contains(json_keys::OFFICES)) {
            auto offices = ParseOffices(obj.at(json_keys::OFFICES).as_array());
            for (const auto& office : offices) {
                map.AddOffice(office);
            }
        }

        return map;
    }

    std::vector<model::Road> MapParser::ParseRoads(const json::array& roadsArray) {
        std::vector<model::Road> roads;
        for (const auto& item : roadsArray) {
            const auto& obj = item.as_object();
            model::Point start{obj.at(json_keys::X0).as_int64(), obj.at(json_keys::Y0).as_int64()};
            if (obj.contains(json_keys::X1)) {
                roads.emplace_back(model::Road::HORIZONTAL, start, obj.at(json_keys::X1).as_int64());
            } else {
                roads.emplace_back(model::Road::VERTICAL, start, obj.at(json_keys::Y1).as_int64());
            }
        }

        return roads;
    }

    std::vector<model::Building> MapParser::ParseBuildings(const json::array& buildingsArray) {
        std::vector<model::Building> buildings;
        for (const auto& item : buildingsArray) {
            const auto& obj = item.as_object();
            model::Rectangle bounds{{obj.at(json_keys::X).as_int64(), obj.at(json_keys::Y).as_int64()},
                                    {obj.at(json_keys::WIDTH).as_int64(), obj.at(json_keys::HEIGHT).as_int64()}};
            buildings.emplace_back(bounds);
        }

        return buildings;
    }

    std::vector<model::Office> MapParser::ParseOffices(const json::array& officesArray) {
        std::vector<model::Office> offices;
        for (const auto& item : officesArray) {
            const auto& obj = item.as_object();
            model::Office::Id id{obj.at(json_keys::ID).as_string().c_str()};
            model::Point position{obj.at(json_keys::X).as_int64(), obj.at(json_keys::Y).as_int64()};
            model::Offset offset{obj.at(json_keys::OFFSET_X).as_int64(), obj.at(json_keys::OFFSET_Y).as_int64()};
            offices.emplace_back(std::move(id), position, offset);
        }

        return offices;
    }

    std::string MapSerializer::SerializeMaps(const std::vector<model::Map>& maps) {
        json::array jsonMaps;
        jsonMaps.reserve(maps.size());
        
        std::transform(maps.cbegin(), maps.cend(), std::back_inserter(jsonMaps), 
            [](const model::Map& map) -> json::object { 
                return SerializeSingleMap(map);
            }
        );

        return boost::json::serialize(json::object{{json_keys::MAPS, std::move(jsonMaps)}});
    }

    std::string MapSerializer::SerializeMapsMainInfo(const std::vector<model::Map>& maps) {
        json::array jsonMaps;
        jsonMaps.reserve(maps.size());
        
        std::transform(maps.cbegin(), maps.cend(), std::back_inserter(jsonMaps), 
            [](const model::Map& map) -> json::object {
                return SerializeSingleMapMainInfo(map);
            }
        );

        return boost::json::serialize(std::move(jsonMaps));
    }

    json::object MapSerializer::SerializeSingleMap(const model::Map& map) {
        json::object jsonMap = SerializeSingleMapMainInfo(map);

        jsonMap[json_keys::ROADS] = SerializeRoads(map.GetRoads());
        jsonMap[json_keys::BUILDINGS] = SerializeBuildings(map.GetBuildings());
        jsonMap[json_keys::OFFICES] = SerializeOffices(map.GetOffices());

        return jsonMap;
    }

    json::object MapSerializer::SerializeSingleMapMainInfo(const model::Map& map) {
        json::object jsonMap;
        jsonMap[json_keys::ID] = *map.GetId();
        jsonMap[json_keys::NAME] = map.GetName();

        return jsonMap;
    }

    json::array MapSerializer::SerializeRoads(const std::vector<model::Road>& roads) {
        json::array jsonRoads;
        for (const auto& road : roads) {
            json::object roadObj;
            roadObj[json_keys::X0] = road.GetStart().x;
            roadObj[json_keys::Y0] = road.GetStart().y;
            // Определяем является ли дорога горизонтальной или вертикальной, 
            // сравнив начальную и конечную точки
            if (road.IsHorizontal()) {
                roadObj[json_keys::X1] = road.GetEnd().x;
            } else {
                roadObj[json_keys::Y1] = road.GetEnd().y;
            }
            jsonRoads.push_back(roadObj);
        }

        return jsonRoads;
    }

    json::array MapSerializer::SerializeBuildings(const std::vector<model::Building>& buildings) {
        json::array jsonBuildings;
        for (const auto& building : buildings) {
            json::object buildingObj;
            buildingObj[json_keys::X] = building.GetBounds().position.x;
            buildingObj[json_keys::Y] = building.GetBounds().position.y;
            buildingObj[json_keys::WIDTH] = building.GetBounds().size.width;
            buildingObj[json_keys::HEIGHT] = building.GetBounds().size.height;
            jsonBuildings.push_back(buildingObj);
        }

        return jsonBuildings;
    }

    json::array MapSerializer::SerializeOffices(const std::vector<model::Office>& offices) {
        json::array jsonOffices;
        for (const auto& office : offices) {
            json::object officeObj;
            officeObj[json_keys::ID] = *office.GetId();
            officeObj[json_keys::X] = office.GetPosition().x;
            officeObj[json_keys::Y] = office.GetPosition().y;
            officeObj[json_keys::OFFSET_X] = office.GetOffset().dx;
            officeObj[json_keys::OFFSET_Y] = office.GetOffset().dy;
            jsonOffices.push_back(officeObj);
        }
        
        return jsonOffices;
    }


    std::string StateSerializer::SerializeStates(const std::vector<model::State>& states) {
        /* json::array states_array;
        for (const auto& state : states) {
            states_array.push_back(SerializeSingleState(state));
        } */
        boost::json::object states_json;

        for (const auto& state: states) {
            states_json[std::to_string(state.id)]  = 
                json_loader::StateSerializer::SerializeSingleState(state);
            std::cout << json::serialize(states_json[std::to_string(state.id)]) << std::endl;
        }

        boost::json::object result = {
            {"players" , states_json}
        };

        return json::serialize(result);
    }

    double format_number(double value, int precision = 9) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision) << value;
        return std::stod(oss.str());
    }

    json::object StateSerializer::SerializeSingleState(const model::State& state) {
        json::object state_obj;
        state_obj[json_keys::POS] = SerializePoint(state.position);
        state_obj[json_keys::SPEED] = SerializeSpeed(state.speed);
        state_obj[json_keys::DIR] = SerializeDirection(state.direction);
        return state_obj;
    }

    json::array StateSerializer::SerializePoint(const model::Pos& point) {
        json::array point_arr;
        point_arr.push_back(format_number(point.x));
        point_arr.push_back(format_number(point.y));
        return point_arr;
    }

    json::array StateSerializer::SerializeSpeed(const model::Speed& speed) {
        json::array speed_arr;
        speed_arr.push_back(speed.x);
        speed_arr.push_back(speed.y);
        return speed_arr;
    }

    std::string StateSerializer::SerializeDirection(model::Direction direction) {
        switch (direction) {
            case model::Direction::NORTH:
                return "U";
            case model::Direction::SOUTH:
                return "D";
            case model::Direction::WEST:
                return "L";
            case model::Direction::EAST:
                return "R";
            case model::Direction::DEFAULT:
            default:
                return "U";
        }
    }

    json::value ParseConfigFile(std::string s) {
        json::value json_value;
        try {
            json_value = json::parse(s);
        } catch (const boost::system::system_error& e) {
            // Обработка ошибок парсинга
            std::cerr << "JSON parsing error: " << e.what() << "\n";
            throw;
        }

        return json_value;
    }

    model::Game LoadGame(const std::filesystem::path& file_path) {
        // Загрузить модель игры из файла
        model::Game game;

        json::value config = ParseConfigFile(util::ReadFromFileIntoString(file_path));

        if (auto val = config.as_object().if_contains(json_keys::CONFIG_DEFAULT_SPEED)) {
            game.SetDefaultDogSpeed((*val).as_double());
        }

        for (auto& map : MapParser::Parse(config)) {
            if (!map.IsDefaultDogSpeedValueConfigured()) {
                map.SetDefaultDogSpeed(game.GetDefaultDogSpeed());
            }
            game.AddMap(std::move(map));
        }

        return game;
    }

}  // namespace json_loader