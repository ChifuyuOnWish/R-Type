#pragma once

#include "Entity.hpp"

#include <cstddef>
#include <vector>

namespace rtype::engine {

class EntityManager
{
public:
    Entity create();
    bool destroy(Entity entity);
    bool isAlive(Entity entity) const noexcept;
    std::size_t aliveCount() const noexcept;

private:
    std::vector<Entity::Generation> _generations;
    std::vector<Entity::Index> _recycledIndices;
    std::size_t _aliveCount = 0;
};

} // namespace rtype::engine
