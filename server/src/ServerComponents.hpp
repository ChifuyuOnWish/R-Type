#pragma once

#include "Messages.hpp"
#include "Registry.hpp"

#include <cstdint>
#include <vector>

namespace rtype::server {

struct NetType
{
    std::uint16_t type = 0;
};

struct NetId
{
    std::uint32_t id = 0;
};

struct GameEventQueue
{
    std::vector<protocol::GameEventMessage> events;
};

inline std::uint32_t netIdOf(const engine::Registry& registry, engine::Entity entity)
{
    return (registry.hasComponent<NetId>(entity) ? registry.getComponent<NetId>(entity).id : 0);
}

} // namespace rtype::server