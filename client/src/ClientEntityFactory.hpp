#pragma once

#include "ClientComponents.hpp"
#include "ecs/Registry.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace rtype::client {

class ClientEntityFactory
{
public:
    ClientEntityFactory();
    bool populate(engine::Registry& registry, engine::Entity entity, std::uint16_t type) const;

private:
    struct Def
    {
        std::string texture;
        SpriteRect source;
        float width = 0.F;
        float height = 0.F;
        int layer = 0;
    };

    std::unordered_map<std::uint16_t, Def> _defs;
};
} // namespace rtype::client
