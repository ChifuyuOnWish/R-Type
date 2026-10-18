#include "systems/DamageSystem.hpp"

#include "Components.hpp"
#include "Registry.hpp"

#include <algorithm>

namespace rtype::engine {

void DamageSystem::update(Registry& registry, float)
{
    registry.each<Hit, Health>([&registry](Entity entity, Hit& hit, Health& health) {
        health.current = std::max(0, health.current - hit.amount);
        registry.removeComponent<Hit>(entity);
    });
}

} // namespace rtype::engine