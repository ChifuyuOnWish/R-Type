#pragma once

#include "ClientComponents.hpp"
#include "ISystem.hpp"
#include "components/Components.hpp"
#include "ecs/Registry.hpp"

#include <algorithm>

namespace rtype::client {

class ClientMotionSystem final : public engine::ISystem
{
public:
    void update(engine::Registry& registry, float dt) override
    {
        registry.each<engine::Position, engine::Velocity, Extrapolation>(
            [dt](engine::Entity, engine::Position& position, engine::Velocity& velocity,
                 Extrapolation& extrapolation) {
                const float step = std::min(dt, extrapolation.left);
                extrapolation.left -= step;
                position.x += velocity.dx * step;
                position.y += velocity.dy * step;
            });
    }
};

} // namespace rtype::client