#include "ClientSync.hpp"

#include "ClientComponents.hpp"
#include "components/Components.hpp"
#include "core/Update.hpp"

#include <iterator>

namespace rtype::client {

ClientSync::ClientSync(engine::Registry& registry) : _registry(registry) {}

void ClientSync::spawn(const protocol::EntitySpawnMessage& message)
{
    if (_destroyedAt.contains(message.entityId) || _byNetId.contains(message.entityId))
        return;

    const engine::Entity entity = _registry.createEntity();
    _registry.addComponent(entity, engine::Position{message.x.toFloat(), message.y.toFloat()});
    _registry.addComponent(entity, engine::Velocity{0.F, 0.F});
    _registry.addComponent(entity, NetworkId{message.entityId});
    if (!_factory.populate(_registry, entity, message.entityType)) {
        _registry.destroyEntity(entity);
        return;
    }
    _byNetId.emplace(message.entityId, entity);
}

void ClientSync::destroy(const protocol::EntityDestroyMessage& message)
{
    _destroyedAt[message.entityId] = _time;
    const auto it = _byNetId.find(message.entityId);
    if (it == _byNetId.end())
        return;
    if (_onDestroyed && _registry.hasComponent<engine::Position>(it->second)) {
        const auto& position = _registry.getComponent<engine::Position>(it->second);
        _onDestroyed(position.x, position.y, message.cause);
    }
    _registry.destroyEntity(it->second);
    _byNetId.erase(it);
}

void ClientSync::applySnapshot(const protocol::SnapshotMessage& message)
{
    if (_hasTick && message.serverTick != _lastTick &&
        message.serverTick - _lastTick >= 0x80000000U)
        return;
    _hasTick = true;
    _lastTick = message.serverTick;

    constexpr float PerSecond = static_cast<float>(1.0 / engine::Update::Step);

    for (const auto& entry : message.entries) {
        const auto it = _byNetId.find(entry.entityId);
        if (it == _byNetId.end()) {
            continue;
        }
        auto& position = _registry.getComponent<engine::Position>(it->second);
        auto& velocity = _registry.getComponent<engine::Velocity>(it->second);
        position.x = entry.x.toFloat();
        position.y = entry.y.toFloat();
        velocity.dx = entry.vx.toFloat() * PerSecond;
        velocity.dy = entry.vy.toFloat() * PerSecond;
    }
}

void ClientSync::update(float dt)
{
    _time += dt;
    for (auto it = _destroyedAt.begin(); it != _destroyedAt.end();)
        it = (_time - it->second > DestroyedMemory) ? _destroyedAt.erase(it) : std::next(it);
}

} // namespace rtype::client
