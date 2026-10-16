#pragma once

#include "Entity.hpp"

#include <cstddef>

namespace rtype::engine {

class IComponentStorage
{
public:
    virtual ~IComponentStorage() = default;

    virtual void remove(Entity entity) = 0;
    virtual bool has(Entity entity) const noexcept = 0;
    virtual std::size_t size() const noexcept = 0;
};

} // namespace rtype::engine
