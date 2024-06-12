#include "util_tests.h"

#include <cassert>

namespace Tests {

    void TestExtractToken() {
        std::vector<std::string> contains_valid_token = {
            "Bearer 21bcec4d2fe13ab845d72a5f19fa57b1\r\n",
            "Bearer {21bcec4d2fe13ab845d72a5f19fa57b1}\r\n",
            "Bearer 21bcec4d2fe13ab845d72a5f19fa57b1 \r\n",
            "Bearer 21bcec4d2fe13ab845d72a5f19fa57b1\r\n",
            "Bearer 21bcec4d2fe13ab845d72a5f19fa57b1\r\n",
            "21bcec4d2fe13ab845d72a5f19fa57b1\r\n",
            "{21bcec4d2fe13ab845d72a5f19fa57b1} \r\n",

        };
        std::vector<std::string> invalid_tokens = {
            "Bearer \r\n",
            "Bearer\r\n",
            "",
            "{a}",
            "\r\n",
        };
        
        std::string result = "21bcec4d2fe13ab845d72a5f19fa57b1";

        for (const auto& str: contains_valid_token) {
            assert(util::ExtractToken(str) == result);
        }

        for (const auto& str: invalid_tokens) {
            assert(util::ExtractToken(str) == "");
        }
    }

    void Tests() {
        TestExtractToken();
    }

}