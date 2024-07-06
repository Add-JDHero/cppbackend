#pragma once

#include "sdk.h"
#include "model.h"
#include "tagged.h"

#include <unordered_map>
#include <boost/json.hpp>

struct LootGeneratorConfig {
    double period = 0;
    double probability = 0;
};

struct MapLootTypes {
    std::unordered_map<model::Map::Id, 
                       std::shared_ptr<boost::json::array>,
                       util::TaggedHasher<model::Map::Id>> mapId_to_lootTypes;
};