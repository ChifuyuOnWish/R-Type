#pragma once

namespace rtype::engine {

class Registry;

class ISystem
{
public:
    virtual ~ISystem() = default;
    virtual void update(Registry& registry, float dt) = 0;
};

} // namespace rtype::engine
