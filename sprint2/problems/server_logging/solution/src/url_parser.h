#pragma once

#include "sdk.h"

#include <string_view>
#include <vector>
#include <string>

namespace url {
    class UrlParser {
    public:
        explicit UrlParser(const std::string& url);

        const std::vector<std::string>& GetComponents() const;
        const std::string& GetRawUrl() const;
        const std::string& GetProtocol() const;

    private:

        const std::string& url_;
        std::string protocol_ = "";
        std::vector<std::string> components_; // Хранение компонентов URL

        // Функция для разбора URL на компоненты, включая протокол
        void parse(std::string_view url);
    };
}