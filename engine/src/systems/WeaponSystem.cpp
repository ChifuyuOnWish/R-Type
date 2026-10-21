#include "WeaponSystem.hpp"

#include "ecs/Registry.hpp"

#include <algorithm>
#include <utility>

namespace rtype::engine {

WeaponSystem::WeaponSystem(SpawnProjectileFn spawn) : _spawn(std::move(spawn)) {}

void WeaponSystem::update(Registry& registry, float dt)
{
    registry.each<Weapon, Position>(
        [this, dt, &registry](Entity shooter, Weapon& weapon, Position& position) {
            weapon.timer = std::max(0.F, weapon.timer - dt);
            const bool wantsToFire = registry.hasComponent<Controllable>(shooter)
                                         ? registry.getComponent<Controllable>(shooter).shoot
                                         : weapon.automatic;
            if (!wantsToFire || weapon.timer > 0.F || !_spawn) {
                return;
            }
            weapon.timer = weapon.cooldown;
            if (weapon.rate > 0.F) {
                weapon.timer = weapon.timer / weapon.rate;
            }
            const Position from = position;
            const unsigned type = weapon.projectileType;
            _spawn(registry, shooter, from, type);
        });
}

} // namespace rtype::engine
