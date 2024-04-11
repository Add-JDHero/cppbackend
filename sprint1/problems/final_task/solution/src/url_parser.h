#pragma once

#include <string_view>
#include <vector>
#include <string>

namespace url {
    class UrlParser {
    public:
        UrlParser(const std::string& url);

        const std::vector<std::string>& getComponents() const;

        const std::string& getProtocol() const;

    private:
        std::string protocol_ = "";
        std::vector<std::string> components_; // Хранение компонентов URL

        // Функция для разбора URL на компоненты, включая протокол
        void parse(std::string_view url);
    };
}