#include "LifetimeSystem.hpp"

#include "Components/Components.hpp"
#include "ecs/Registry.hpp"

namespace rtype::engine {

void LifetimeSystem::update(Registry& registry, float dt)
{
    registry.each<Lifetime>([dt, &registry](Entity entity, Lifetime& lifetime) {
        lifetime.remaining -= dt;
        if (lifetime.remaining <= 0.F) {
            registry.destroyLater(entity);
        }
    });
}

} // namespace rtype::engine
