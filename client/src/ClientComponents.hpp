#pragma once

#include <cstdint>
#include <string>

namespace rtype::client {

struct SpriteRect
{
    float x = 0.F;
    float y = 0.F;
    float w = 0.F;
    float h = 0.F;
};

enum class SpriteMotion
{
    None,
    Plane,
    Bat,
    Slug,
    Hover,
    AlliedFire,
    EnemyFire
};

struct Sprite
{
    std::string textureId;
    SpriteRect source;
    float width = 0.F;
    float height = 0.F;
    int layer = 0;
    SpriteMotion motion = SpriteMotion::None;
    float shotPoseRemaining = 0.0F;
    bool flipY = false; // drawn upside down, e.g. a slug walking on the ceiling
};

struct NetworkId
{
    std::uint32_t id = 0;
};

struct PlayerSlot
{
    std::uint8_t index = 0;
};

struct Extrapolation
{
    float left = 0.F;
    float offsetX = 0.F;
    float offsetY = 0.F;
};

} // namespace rtype::client