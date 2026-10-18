#pragma once

#include "EventDispatcher.hpp"
#include "ISystem.hpp"

namespace rtype::engine {

// Applies Hit components to Health. With a dispatcher, publishes a DamageEvent per hit and an
// EntityDiedEvent when an entity's health reaches 0.
class DamageSystem : public ISystem
{
public:
    explicit DamageSystem(EventDispatcher* events = nullptr);
    void update(Registry& registry, float dt) override;

private:
    EventDispatcher* _events;
};

} // namespace rtype::engine
