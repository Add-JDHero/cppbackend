#include "url_parser.h"

namespace url {
    UrlParser::UrlParser(const std::string& url) {
        parse(url);
    }

    const std::vector<std::string>& UrlParser::getComponents() const {
        return components_;
    }

    const std::string& UrlParser::getProtocol() const {
        return protocol_;
    }

    // Функция для разбора URL на компоненты, включая протокол
    void UrlParser::parse(std::string_view url) {
        auto protocolSeparatorPos = url.find("://");
        if (protocolSeparatorPos != std::string_view::npos) {
            protocol_ = url.substr(0, protocolSeparatorPos);
            url.remove_prefix(protocolSeparatorPos + 3); // "+3" для пропуска "://"
        }

        std::string component;
        while (true) {
            auto delimPos = url.find('/');
            if (delimPos == std::string_view::npos) {
                if (!url.empty()) components_.push_back(std::string(url));
                break;
            }
            component = url.substr(0, delimPos);
            if (!component.empty()) {
                components_.push_back(component);
            }
            url.remove_prefix(delimPos + 1); // "+1" для пропуска '/'
        }
    }
}