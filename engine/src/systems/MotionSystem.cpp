#include "MotionSystem.hpp"

#include "Components.hpp"
#include "Registry.hpp"

namespace rtype::engine {

void MotionSystem::update(Registry& registry, float dt)
{
    registry.each<Position, Velocity>([dt](const Entity&, Position& position, Velocity& velocity) {
        position.x += velocity.dx * dt;
        position.y += velocity.dy * dt;
    });
}

} // namespace rtype::engine