#pragma once

#include "ISystem.hpp"
#include "components/Components.hpp"
#include "ecs/Registry.hpp"

namespace rtype::client {

class ClientMotionSystem final : public engine::ISystem
{
public:
    void update(engine::Registry& registry, float dt) override
    {
        registry.each<engine::Position, engine::Velocity>(
            [dt](engine::Entity, engine::Position& position, engine::Velocity& velocity) {
                position.x += velocity.dx * dt;
                position.y += velocity.dy * dt;
            });
    }
};

} // namespace rtype::client