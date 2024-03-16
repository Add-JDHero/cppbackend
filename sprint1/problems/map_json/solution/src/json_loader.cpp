#include "json_loader.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>

const int BUFF_SIZE = 1024;
#define BOOST_BEAST_USE_STD_STRING_VIEW

namespace json_loader {
    namespace json = boost::json;
    using namespace std::literals;

        std::vector<model::Map> MapParser::Parse(const json::value& jsonVal) {
            std::vector<model::Map> maps;
            auto mapsArray = jsonVal.at("maps").as_array();
            for (const auto& mapVal : mapsArray) {
                maps.push_back(ParseSingleMap(mapVal.as_object()));
            }
            return maps;
        }

        model::Map MapParser::ParseSingleMap(const json::object& obj) {
            model::Map::Id mapId{obj.at("id").as_string().c_str()};
            std::string name = obj.at("name").as_string().c_str();
            model::Map map(std::move(mapId), std::move(name));

            if (obj.contains("roads")) {
                auto roads = ParseRoads(obj.at("roads").as_array());
                for (const auto& road : roads) {
                    map.AddRoad(road);
                }
            }

            if (obj.contains("buildings")) {
                auto buildings = ParseBuildings(obj.at("buildings").as_array());
                for (const auto& building : buildings) {
                    map.AddBuilding(building);
                }
            }

            if (obj.contains("offices")) {
                auto offices = ParseOffices(obj.at("offices").as_array());
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
                model::Point start{obj.at("x0").as_int64(), obj.at("y0").as_int64()};
                if (obj.contains("x1")) {
                    roads.emplace_back(model::Road::HORIZONTAL, start, obj.at("x1").as_int64());
                } else {
                    roads.emplace_back(model::Road::VERTICAL, start, obj.at("y1").as_int64());
                }
            }
            return roads;
        }

        std::vector<model::Building> MapParser::ParseBuildings(const json::array& buildingsArray) {
            std::vector<model::Building> buildings;
            for (const auto& item : buildingsArray) {
                const auto& obj = item.as_object();
                model::Rectangle bounds{{obj.at("x").as_int64(), obj.at("y").as_int64()},
                                        {obj.at("w").as_int64(), obj.at("h").as_int64()}};
                buildings.emplace_back(bounds);
            }
            return buildings;
        }

        std::vector<model::Office> MapParser::ParseOffices(const json::array& officesArray) {
            std::vector<model::Office> offices;
            for (const auto& item : officesArray) {
                const auto& obj = item.as_object();
                model::Office::Id id{obj.at("id").as_string().c_str()};
                model::Point position{obj.at("x").as_int64(), obj.at("y").as_int64()};
                model::Offset offset{obj.at("offsetX").as_int64(), obj.at("offsetY").as_int64()};
                offices.emplace_back(std::move(id), position, offset);
            }
            return offices;
        }

        std::string MapSerializer::SerializeMaps(const std::vector<model::Map>& maps) {
            json::array jsonMaps;
            for (const auto& map : maps) {
                jsonMaps.push_back(SerializeSingleMap(map));
            }
            return boost::json::serialize(json::object{{"maps", std::move(jsonMaps)}});
        }

        std::string MapSerializer::SerializeMapsMainInfo(const std::vector<model::Map>& maps) {
            json::array jsonMaps;
            for (const auto& map : maps) {
                jsonMaps.push_back(SerializeSingleMapMainInfo(map));
            }
            return boost::json::serialize(json::object{{"maps", std::move(jsonMaps)}});
        }

        json::object MapSerializer::SerializeSingleMap(const model::Map& map) {
            json::object jsonMap;
            jsonMap["id"] = *map.GetId();
            jsonMap["name"] = map.GetName();
            jsonMap["roads"] = SerializeRoads(map.GetRoads());
            jsonMap["buildings"] = SerializeBuildings(map.GetBuildings());
            jsonMap["offices"] = SerializeOffices(map.GetOffices());
            return jsonMap;
        }

        json::object MapSerializer::SerializeSingleMapMainInfo(const model::Map& map) {
            json::object jsonMap;
            jsonMap["id"] = *map.GetId();
            jsonMap["name"] = map.GetName();
            return jsonMap;
        }

        json::array MapSerializer::SerializeRoads(const std::vector<model::Road>& roads) {
            json::array jsonRoads;
            for (const auto& road : roads) {
                json::object roadObj;
                roadObj["x0"] = road.GetStart().x;
                roadObj["y0"] = road.GetStart().y;
                // Determine if road is horizontal or vertical by comparing start and end points
                if (road.IsHorizontal()) {
                    roadObj["x1"] = road.GetEnd().x;
                } else {
                    roadObj["y1"] = road.GetEnd().y;
                }
                jsonRoads.push_back(roadObj);
            }
            return jsonRoads;
        }

        json::array MapSerializer::SerializeBuildings(const std::vector<model::Building>& buildings) {
            json::array jsonBuildings;
            for (const auto& building : buildings) {
                json::object buildingObj;
                buildingObj["x"] = building.GetBounds().position.x;
                buildingObj["y"] = building.GetBounds().position.y;
                buildingObj["w"] = building.GetBounds().size.width;
                buildingObj["h"] = building.GetBounds().size.height;
                jsonBuildings.push_back(buildingObj);
            }
            return jsonBuildings;
        }

        json::array MapSerializer::SerializeOffices(const std::vector<model::Office>& offices) {
            json::array jsonOffices;
            for (const auto& office : offices) {
                json::object officeObj;
                officeObj["id"] = *office.GetId();
                officeObj["x"] = office.GetPosition().x;
                officeObj["y"] = office.GetPosition().y;
                officeObj["offsetX"] = office.GetOffset().dx;
                officeObj["offsetY"] = office.GetOffset().dy;
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

        for (auto& map: MapParser::Parse(config)) {
            game.AddMap(std::move(map));
        }

        return game;
    }

}  // namespace json_loader