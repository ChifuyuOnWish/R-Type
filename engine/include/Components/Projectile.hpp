#pragma once

#include "ecs/Entity.hpp"

namespace rtype::engine {

struct Projectile
{
    Entity owner;
    float lifetime = 0.F;
    unsigned pierceCount = 0;
};

} // namespace rtype::engine
