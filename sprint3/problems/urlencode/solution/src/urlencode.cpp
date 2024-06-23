#include "urlencode.h"
#include <iostream>
#include <sstream>

/*
 * URL-кодирует строку str.
 * Пробел заменяется на +,
 * Символы, отличные от букв английского алфавита, цифр и -._~ а также зарезервированные символы
 * заменяются на их %-кодированные последовательности.
 * Зарезервированные символы: !#$&'()*+,/:;=?@[]
 */

std::string UrlEncode(std::string_view str) {
    std::string answ;
    for (auto& p : str) {
        if (p == ' ') {
            answ.push_back('+');
            continue;
        }
        if (static_cast<int>(p) < 32 || p == '!') {
            std::stringstream ss;
            ss << std::hex << static_cast<int>(p);
            std::string s = ss.str();
            if (s.size() == 1)
                s = "0" + s;
            answ += ("%" + s);
            continue;
        }
        answ.push_back(p);
    }

    return answ;
}
