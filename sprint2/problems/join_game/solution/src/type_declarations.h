#pragma once

#include "sdk.h"

#include <variant>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>

using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;

namespace http_handler {
    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;

    using FileResponse = http::response<http::file_body>;
    using ResponseVariant = std::variant<StringResponse, FileResponse>;
}