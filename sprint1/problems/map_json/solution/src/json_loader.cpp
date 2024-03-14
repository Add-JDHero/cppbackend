#include "json_loader.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>
#include <boost/json.hpp>

static const int BUFF_SIZE = 1024;

namespace json_loader {
    std::string ReadFromFileIntoString(std::ifstream& file) {
        char buff[BUFF_SIZE];
        std::string str;
        std::streamsize count = 0;
        while ( file.read(buff, BUFF_SIZE) || (count = file.gcount()) ) {
            str.append(buff, count);
        }

        return str;
    }

    boost::json::value ParseConfigFile(std::string s) {
        boost::json::value json_value;
        try {
            json_value = boost::json::parse(s);
        } catch (const boost::system::system_error& e) {
            // Обработка ошибок парсинга
            std::cerr << "JSON parsing error: " << e.what() << "\n";
            throw;
        }

        return json_value;
    }

    model::Game LoadGame(const std::filesystem::path& json_path) {
        // Загрузить содержимое файла json_path, например, в виде строки
        // Распарсить строку как JSON, используя boost::json::parse
        // Загрузить модель игры из файла
        model::Game game;
        std::ifstream config_file(json_path, std::ios::in | std::ios::binary);

        if (!config_file.is_open()) {
            throw std::runtime_error("Failed to open file: " + json_path.string());
        }

        boost::json::value config = ParseConfigFile(ReadFromFileIntoString(config_file));

        auto maps = config.at("maps").as_array();

        for (const auto& item: maps) {
            auto id = item.at("id").as_string();
            auto name = item.at("name").as_string();

            model::Map::Id map_id(std::string(std::move(id)));

            model::Map Map(map_id, std::string(name));
            game.AddMap(std::move(Map));
        }

        return game;
    }

}  // namespace json_loader