#pragma once

namespace rtype::engine {

struct Weapon
{
    float cooldown = 0.F;
    float timer = 0.F;
    unsigned projectileType = 0;
    float rate = 1.F;
    bool automatic = false;
};

// namespace rtype::engine
