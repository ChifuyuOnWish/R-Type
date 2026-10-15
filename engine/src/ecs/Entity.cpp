#include "Entity.hpp"

namespace rtype::engine {

Entity::Entity(Index index, Generation generation) noexcept : _index(index), _generation(generation)
{
}

Entity::Index Entity::index() const noexcept
{
    return _index;
}

Entity::Generation Entity::generation() const noexcept
{
    return _generation;
}

} // namespace rtype::engine
