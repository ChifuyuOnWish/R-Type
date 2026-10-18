#include "systems/BoundsSystem.hpp"

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
            position.x = std::clamp(position.x, _area.minX, _area.maxX);
            position.y = std::clamp(position.y, _area.minY, _area.maxY);
            return;
        }
        if (isOutside(position, _area, _margin)) {
            registry.destroyLater(entity);
        }
    });
}

} // namespace rtype::engine
