#pragma once

#include "Entity.hpp"

namespace rtype::engine {

struct CollisionEvent
{
    Entity a;
    Entity b;
};

struct DamageEvent
{
    Entity victim;
    int amount = 0;
};

struct EntityDiedEvent
{
    Entity entity;
};

struct EntitySpawnedEvent
{
    Entity entity;
};

} // namespace rtype::engine
