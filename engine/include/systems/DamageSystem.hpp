#pragma once

#include "EventDispatcher.hpp"
#include "ISystem.hpp"

namespace rtype::engine {

class DamageSystem : public ISystem
{
public:
    explicit DamageSystem(EventDispatcher* events = nullptr);
    void update(Registry& registry, float dt) override;

private:
    EventDispatcher* _events;
};

} // namespace rtype::engine
