#include "DeathSystem.hpp"

#include "Components.hpp"
#include "Registry.hpp"

namespace rtype::engine {

void DeathSystem::update(Registry& registry, float)
{
    registry.each<Health>([&registry](Entity entity, Health& health) {
        if (health.current <= 0) {
            registry.destroyLater(entity);
        }
    });
}

} // namespace rtype::engine
