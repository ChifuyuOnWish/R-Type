#pragma once

#include "ClientEntityFactory.hpp"
#include "Messages.hpp"
#include "ecs/Registry.hpp"

#include <cstdint>
#include <functional>
#include <unordered_map>

namespace rtype::client {

class ClientSync
{
public:
    using DestroyedHandler = std::function<void(float x, float y, protocol::DestroyCause cause)>;

    explicit ClientSync(engine::Registry& registry);

    void onEntityDestroyed(DestroyedHandler fn) { _onDestroyed = std::move(fn); }
    void spawn(const protocol::EntitySpawnMessage& message);
    void destroy(const protocol::EntityDestroyMessage& message);
    void applySnapshot(const protocol::SnapshotMessage& message);
    void update(float dt);

private:
    static constexpr float DestroyedMemory = 5.F;

    engine::Registry& _registry;
    ClientEntityFactory _factory;
    std::unordered_map<std::uint32_t, engine::Entity> _byNetId;
    std::unordered_map<std::uint32_t, float> _destroyedAt;
    DestroyedHandler _onDestroyed;
    float _time = 0.F;
    std::uint32_t _lastTick = 0;
    bool _hasTick = false;
};
} // namespace rtype::client
