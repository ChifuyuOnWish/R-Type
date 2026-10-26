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

struct Sprite
{
    std::string textureId;
    SpriteRect source;
    float width = 0.F;
    float height = 0.F;
    int layer = 0;
};

struct NetworkId
{
    std::uint32_t id = 0;
};

struct PlayerSlot
{
    std::uint8_t index = 0;
};

} // namespace rtype::client