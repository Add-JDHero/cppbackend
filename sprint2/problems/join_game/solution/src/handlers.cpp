#include "handlers.h"

HTTPResponseMaker::HTTPResponseMaker(std::function<ResponseMaker> handler)
    : handler_(std::move(handler)) {
}

http_handler::ResponseVariant HTTPResponseMaker::Invoke(const http_handler::StringRequest& req, 
                                                                     JsonResponseHandler json_response) {
    return handler_(req, json_response);
}