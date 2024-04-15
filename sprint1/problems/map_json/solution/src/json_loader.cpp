#include "json_loader.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>

const int BUFF_SIZE = 1024;
#define BOOST_BEAST_USE_STD_STRING_VIEW

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

    std::string ReadFromFileIntoString(std::ifstream& file) {
        char buff[BUFF_SIZE];
        std::string str;
        std::streamsize count = 0;
        while ( file.read(buff, BUFF_SIZE) || (count = file.gcount()) ) {
            str.append(buff, count);
        }

        return str;
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

    model::Game LoadGame(const std::filesystem::path& json_path) {
        // Загрузить содержимое файла json_path, например, в виде строки
        // Распарсить строку как JSON, используя json::parse
        // Загрузить модель игры из файла
        model::Game game;
        std::ifstream config_file(json_path, std::ios::in | std::ios::binary);

        if (!config_file.is_open()) {
            throw std::runtime_error("Failed to open file: " + json_path.string());
        }

        json::value config = ParseConfigFile(ReadFromFileIntoString(config_file));

        for (auto& map : MapParser::Parse(config)) {
            game.AddMap(std::move(map));
        }

        return game;
    }

}  // namespace json_loader