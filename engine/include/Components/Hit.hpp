#pragma once

#include "ecs/Entity.hpp"

namespace rtype::engine {

struct Hit
{
    int amount = 0;
    Entity source{};
};

} // namespace rtype::engine
