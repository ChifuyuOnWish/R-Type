#include "systems/InputSystem.hpp"

#include "Components.hpp"
#include "Registry.hpp"

namespace rtype::engine {

InputSystem::InputSystem(float speed) : _speed(speed) {}

void InputSystem::update(Registry& registry, float)
{
    registry.each<Controllable, Velocity>([this](const Entity&, Controllable& input,
                                                 Velocity& velocity) {
        velocity.dx = (static_cast<float>(input.right) - static_cast<float>(input.left)) * _speed;
        velocity.dy = (static_cast<float>(input.down) - static_cast<float>(input.up)) * _speed;
    });
}

} // namespace rtype::engine
