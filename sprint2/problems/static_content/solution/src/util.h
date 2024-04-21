#pragma once 

#include <filesystem>
#include <fstream>
#include <boost/json.hpp>
#include <boost/beast.hpp>

const int BUFF_SIZE = 1024;

namespace util {

    namespace beast = boost::beast;
    namespace http = beast::http;

    std::string ReadFromFileIntoString(const std::filesystem::path& file_path);

    beast::string_view MimeType(beast::string_view path);

    std::string TestFunc(const std::filesystem::path& file_path);

    std::string ExtractFileExtension(const std::filesystem::path& path);

    http::response<http::file_body> ReadStaticFile(const std::filesystem::path& file_path);

}