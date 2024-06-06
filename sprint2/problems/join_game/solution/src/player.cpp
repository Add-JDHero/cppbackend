#include "player.h"
#include <iomanip>


namespace app {
	Token PlayerTokens::GenerateToken() {
        std::stringstream ss;

        std::uniform_int_distribution<uint64_t> dist;
        uint64_t part1 = dist(generator1_);
        uint64_t part2 = dist(generator2_);

        ss << std::hex << std::setfill('0') 
           << std::setw(16) << part1 << std::setw(16) << part2;
        return Token{ss.str()};
    }
}