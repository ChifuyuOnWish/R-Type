#pragma once

#include <cstdint>

namespace rtype::protocol {

enum class MessageType : std::uint8_t
{
    Input = 0x10,
    PlayerJoined = 0x11,
    PlayerLeft = 0x12,
    EntitySpawn = 0x13,
    EntityDestroy = 0x14,
    Snapshot = 0x15,
    GameEvent = 0x16,
};

} // namespace rtype::protocol