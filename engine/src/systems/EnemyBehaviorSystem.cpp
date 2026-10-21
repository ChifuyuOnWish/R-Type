#include "EnemyBehaviorSystem.hpp"

#include "components/Components.hpp"
#include "ecs/Registry.hpp"

namespace rtype::engine {

void EnemyBehaviorSystem::update(Registry& registry, float)
{
    registry.each<EnemyBehavior, Velocity>([](Entity, EnemyBehavior& behavior, Velocity& velocity) {
        if (behavior.pattern == EnemyPattern::Straight) {
            velocity.dx = -behavior.speed;
            velocity.dy = 0.F;
        }
    });
}

} // namespace rtype::engine