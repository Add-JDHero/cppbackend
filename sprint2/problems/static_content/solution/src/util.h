#pragma once 

#include <filesystem>
#include <fstream>
#include <boost/json.hpp>
#include <boost/beast.hpp>

const int BUFF_SIZE = 1024;

namespace util {

    std::string ReadFromFileIntoString(const std::filesystem::path& file_path);

}