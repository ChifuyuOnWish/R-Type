#include "systems/WeaponSystem.hpp"

#include "Registry.hpp"

#include <algorithm>
#include <utility>

namespace rtype::engine {

WeaponSystem::WeaponSystem(SpawnProjectileFn spawn) : _spawn(std::move(spawn)) {}

void WeaponSystem::update(Registry& registry, float dt)
{
    registry.each<Weapon, Controllable, Position>(
        [this, dt, &registry](Entity shooter, Weapon& weapon, Controllable& input,
                              Position& position) {
            weapon.timer = std::max(0.F, weapon.timer - dt);
            if (!input.shoot || weapon.timer > 0.F || !_spawn) {
                return;
            }
            weapon.timer = weapon.cooldown;
            if (weapon.rate > 0.F) {
                weapon.timer = weapon.timer / weapon.rate;
            }
            _spawn(registry, shooter, position, weapon.projectileType);
        });
}

} // namespace rtype::engine
