#include "systems/DamageSystem.hpp"

#include "Components.hpp"
#include "Events.hpp"
#include "Registry.hpp"

#include <algorithm>

namespace rtype::engine {

DamageSystem::DamageSystem(EventDispatcher* events) : _events(events) {}

void DamageSystem::update(Registry& registry, float dt)
{
    registry.each<Invincibility>(
        [dt](Entity, Invincibility& inv) { inv.remaining = std::max(0.F, inv.remaining - dt); });
    registry.each<Hit, Health>([this, &registry](Entity entity, Hit& hit, Health& health) {
        const Hit taken = hit;
        registry.removeComponent<Hit>(entity);
        if (registry.hasComponent<Invincibility>(entity) &&
            registry.getComponent<Invincibility>(entity).remaining > 0.F) {
            return;
        }
        const bool wasAlive = health.current > 0;
        health.current = std::max(0, health.current - taken.amount);
        if (_events == nullptr) {
            return;
        }
        _events->publish(DamageEvent{entity, taken.amount, taken.source});
        if (wasAlive && health.current == 0) {
            _events->publish(EntityDiedEvent{entity, taken.source});
        }
    });
}

} // namespace rtype::engine
