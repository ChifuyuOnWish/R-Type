#pragma once

#include "ecs/Registry.hpp"

#include <cstdint>

namespace rtype::server {

struct NetType
{
    std::uint16_t type = 0;
};

struct NetId
{
    std::uint32_t id = 0;
};

inline std::uint32_t netIdOf(const engine::Registry& registry, engine::Entity entity)
{
    return (registry.hasComponent<NetId>(entity) ? registry.getComponent<NetId>(entity).id : 0);
}

} // namespace rtype::server
