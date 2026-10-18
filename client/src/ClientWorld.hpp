#pragma once

#include "Messages.hpp"

#include <cstdint>
#include <unordered_map>

namespace rtype::client {

struct WorldEntity
{
    std::uint16_t type = 0;
    float x = 0.0F;
    float y = 0.0F;
};

class ClientWorld
{
public:
    void spawn(const protocol::EntitySpawnMessage& message)
    {
        _entities.try_emplace(message.entityId, WorldEntity{message.entityType, message.x.toFloat(),
                                                            message.y.toFloat()});
    }

    void destroy(const protocol::EntityDestroyMessage& message)
    {
        _entities.erase(message.entityId);
    }

    void applySnapshot(const protocol::SnapshotMessage& message)
    {
        if (_hasTick && message.serverTick != _lastTick &&
            message.serverTick - _lastTick >= 0x80000000U)
            return;
        _hasTick = true;
        _lastTick = message.serverTick;
        for (const auto& entry : message.entries) {
            const auto it = _entities.find(entry.entityId);
            if (it == _entities.end())
                continue;
            it->second.x = entry.x.toFloat();
            it->second.y = entry.y.toFloat();
        }
    }

    [[nodiscard]] const std::unordered_map<std::uint32_t, WorldEntity>& entities() const noexcept
    {
        return _entities;
    }

private:
    std::unordered_map<std::uint32_t, WorldEntity> _entities;
    std::uint32_t _lastTick = 0;
    bool _hasTick = false;
};

} // namespace rtype::client
