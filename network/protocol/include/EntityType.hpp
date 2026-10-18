#pragma once

#include <cstdint>

namespace rtype::protocol {

enum class EntityType : std::uint16_t
{
    Player1 = 1,
    Player2 = 2,
    Player3 = 3,
    Player4 = 4,
    EnemyBasic = 10,
    PlayerMissile = 20,
    EnemyMissile = 21,
};

inline EntityType playerShipType(std::uint32_t clientId)
{
    return static_cast<EntityType>(clientId);
}

} // namespace rtype::protocol
