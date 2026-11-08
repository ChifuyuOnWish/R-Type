#pragma once

namespace rtype::server {

struct Tile
{
    float size = 60.F;
    bool solid = true;
    int contactDamage = 0;
};

struct TerrainContact
{
    float cooldown = 0.F;
};

inline constexpr float TerrainHitCooldown = 1.F;

} // namespace rtype::server
