#pragma once

#include "ClientSync.hpp"
#include "ISystem.hpp"
#include "Messages.hpp"
#include "ecs/Registry.hpp"

#include <memory>
#include <vector>

namespace rtype::client {

class ClientWorld
{
public:
    ClientWorld();

    void spawn(const protocol::EntitySpawnMessage& message) { _sync.spawn(message); }
    void destroy(const protocol::EntityDestroyMessage& message) { _sync.destroy(message); }
    void applySnapshot(const protocol::SnapshotMessage& message) { _sync.applySnapshot(message); }

    void update(float dt);

    [[nodiscard]] engine::Registry& registry() noexcept { return _registry; }
    [[nodiscard]] ClientSync& sync() noexcept { return _sync; }

private:
    engine::Registry _registry;
    ClientSync _sync{_registry};
    std::vector<std::unique_ptr<engine::ISystem>> _systems;
};

} // namespace rtype::client
