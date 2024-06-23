#include "urldecode.h"

#include <charconv>
#include <stdexcept>

std::string UrlDecode(std::string_view url_path) {
    std::string answ;
    auto it = url_path.begin();
    while (it != url_path.end()) {
        if (*it == '+') {
            answ.push_back(' ');
            ++it;
            continue;
        }
        if (*it == '%') {
            std::string temp;
            temp.push_back(*(++it));
            temp.push_back(*(++it));
            char* p_end{};
            try {
                answ.push_back(static_cast<char>(std::strtol(temp.data(), &p_end, 16)));
            }
            catch (...) {
                return "invalid_argument";
            }
            ++it;
            continue;
        }
        answ.push_back(*(it++));
    }
    return answ;
}
