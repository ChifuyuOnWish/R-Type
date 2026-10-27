#include "ClientWorld.hpp"

#include "systems/ClientMotionSystem.hpp"

namespace rtype::client {

ClientWorld::ClientWorld()
{
    _systems.push_back(std::make_unique<ClientMotionSystem>());
}

void ClientWorld::update(float dt)
{
    _sync.update(dt);
    for (auto& system : _systems) {
        system->update(_registry, dt);
    }
    _registry.flushDestroyed();
}
} // namespace rtype::client
