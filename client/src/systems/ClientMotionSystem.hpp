#pragma once

#include "ClientComponents.hpp"
#include "ISystem.hpp"
#include "components/Components.hpp"
#include "ecs/Registry.hpp"

#include <algorithm>
#include <cmath>

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
                const float remaining = std::exp(-dt / 0.035F);
                extrapolation.offsetX *= remaining;
                extrapolation.offsetY *= remaining;
            });
        registry.each<Sprite>([dt](engine::Entity, Sprite& sprite) {
            if (sprite.shotPoseRemaining > 0.0F)
                sprite.shotPoseRemaining = std::max(0.0F, sprite.shotPoseRemaining - dt);
        });
    }
};

} // namespace rtype::client