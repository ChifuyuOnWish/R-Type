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
    EnemyFire,
    Boss
};

inline constexpr float HitFlashDuration = 0.30F;
inline constexpr float HitFlashBlinks = 2.0F;
inline constexpr float HitFlashStrength = 0.75F;
inline constexpr float HitPopScale = 0.15F;

struct Sprite
{
    std::string textureId;
    SpriteRect source;
    float width = 0.F;
    float height = 0.F;
    int layer = 0;
    SpriteMotion motion = SpriteMotion::None;
    float shotPoseRemaining = 0.0F;
    bool flipY = false;
    float hitFlashRemaining = 0.0F;
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

struct Dying
{
    float elapsed = 0.F;
};

inline constexpr float DeathDuration = 0.8F;

} // namespace rtype::client