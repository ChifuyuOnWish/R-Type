#include "BoundsSystem.hpp"

#include "Components.hpp"
#include "Registry.hpp"

#include <algorithm>

namespace rtype::engine {

BoundsSystem::BoundsSystem(BoundsArea area, float margin) : _area(area), _margin(margin) {}

namespace {

bool isOutside(const Position& p, const BoundsArea& area, float margin)
{
    return p.x < area.minX - margin || p.x > area.maxX + margin || p.y < area.minY - margin ||
           p.y > area.maxY + margin;
}

} // namespace

void BoundsSystem::update(Registry& registry, float)
{
    registry.each<Position>([this, &registry](const Entity& entity, Position& position) {
        if (registry.hasComponent<Controllable>(entity)) {
            float halfW = 0.F;
            float halfH = 0.F;
            if (registry.hasComponent<Hitbox>(entity)) {
                halfW = registry.getComponent<Hitbox>(entity).width / 2.F;
                halfH = registry.getComponent<Hitbox>(entity).height / 2.F;
            }
            position.x = std::clamp(position.x, _area.minX + halfW, _area.maxX - halfW);
            position.y = std::clamp(position.y, _area.minY + halfH, _area.maxY - halfH);
            return;
        }
        if (isOutside(position, _area, _margin)) {
            registry.destroyLater(entity);
        }
    });
}

} // namespace rtype::engine
