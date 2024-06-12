#pragma once 
#include "sdk.h"
#include "type_declarations.h"

#include <boost/beast.hpp>
#include <filesystem>
#include <fstream>

const int BUFF_SIZE = 1024;

namespace util {

    namespace beast = boost::beast;
    namespace http = beast::http;

    std::string UrlDecode(const std::string& s);

    std::string ReadFromFileIntoString(const std::filesystem::path& file_path);

    std::string_view MimeType(std::string_view path);
 
    std::string TestFunc(const std::filesystem::path& file_path);

    std::string ExtractFileExtension(const std::filesystem::path& path);

    http::response<http::file_body> ReadStaticFile(const std::filesystem::path& file_path);

    std::string ExtractToken(const std::string& auth_header);

}