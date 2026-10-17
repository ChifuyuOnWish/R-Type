#include "Registry.hpp"

namespace rtype::engine {

Entity Registry::createEntity()
{
    return _entities.create();
}

bool Registry::destroyEntity(Entity entity)
{
    if (!_entities.isAlive(entity)) {
        return false;
    }
    for (auto& entry : _storages) {
        entry.second->remove(entity);
    }
    return _entities.destroy(entity);
}

bool Registry::isAlive(Entity entity) const noexcept
{
    return _entities.isAlive(entity);
}

std::size_t Registry::aliveCount() const noexcept
{
    return _entities.aliveCount();
}

void Registry::destroyLater(Entity entity)
{
    _pendingDestroy.push_back(entity);
}

void Registry::flushDestroyed()
{
    for (const Entity entity : _pendingDestroy) {
        destroyEntity(entity);
    }
    _pendingDestroy.clear();
}

} // namespace rtype::engine
