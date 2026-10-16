#include "EntityManager.hpp"

namespace rtype::engine {

Entity EntityManager::create()
{
    if (!_recycledIndices.empty()) {
        const Entity::Index index = _recycledIndices.back();
        _recycledIndices.pop_back();
        _aliveCount++;
        return Entity(index, _generations[index]);
    }
    const auto index = static_cast<Entity::Index>(_generations.size());
    _generations.push_back(0);
    _aliveCount++;
    return Entity(index, 0);
}

bool EntityManager::destroy(Entity entity)
{
    if (!isAlive(entity)) {
        return false;
    }
    _generations[entity.index()]++;
    _recycledIndices.push_back(entity.index());
    _aliveCount--;
    return true;
}

bool EntityManager::isAlive(Entity entity) const noexcept
{
    if (entity.isNull() || entity.index() >= _generations.size()) {
        return false;
    }
    return _generations[entity.index()] == entity.generation();
}

std::size_t EntityManager::aliveCount() const noexcept
{
    return _aliveCount;
}

} // namespace rtype::engine
