#pragma once

#include "sdk.h"
#include "tagged.h"
#include "model.h"
#include <unordered_map>
#include <boost/json.hpp>

struct LootGeneratorConfig {
    double period = 0;
    double probability = 0;
};
