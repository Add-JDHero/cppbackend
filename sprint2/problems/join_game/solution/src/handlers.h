#pragma once

#include "sdk.h"
#include "type_declarations.h"

#include <boost/beast/http.hpp>
#include <functional>
#include <string>
#include <string_view>
#include <any>

namespace beast = boost::beast;
namespace http = beast::http;

using JsonResponseHandler = 
        std::function<http_handler::StringResponse(http::status, std::string, std::string_view)>;

class HandlerBase {
public:
    virtual ~HandlerBase() = default;
    virtual http_handler::ResponseVariant Invoke(const http_handler::StringRequest& req, 
                                                 JsonResponseHandler json_response) = 0;
};

class HTTPResponseMaker : public HandlerBase {
    using ResponseMaker = http_handler::ResponseVariant(const http_handler::StringRequest&, 
                                                        JsonResponseHandler);
public:
    HTTPResponseMaker(std::function<ResponseMaker> handler);

    http_handler::ResponseVariant Invoke(const http_handler::StringRequest& req, 
                                                 JsonResponseHandler json_response) override;

private:
    std::function<ResponseMaker> handler_;
};
