#include "monsters/Targeting.hpp"

#include <cmath>
#include <limits>

namespace rtype::server {

using namespace engine;

std::vector<Position> shipPositions(Registry& registry)
{
    std::vector<Position> ships;

    registry.each<Controllable, Position>(
        [&ships](Entity, Controllable&, Position& p) { ships.push_back(p); });
    return ships;
}

std::optional<Position> nearest(const std::vector<Position>& ships, Position from)
{
    std::optional<Position> best;
    float bestDist = std::numeric_limits<float>::max();
    for (const Position& p : ships) {
        const float dx = p.x - from.x;
        const float dy = p.y - from.y;
        const float dist = dx * dx + dy * dy;
        if (dist < bestDist) {
            bestDist = dist;
            best = p;
        }
    }
    return best;
}

std::optional<Position> nearestShip(Registry& registry, Position from)
{
    return nearest(shipPositions(registry), from);
}

Velocity towards(Position from, Position to, float speed)
{
    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1.F)
        return Velocity{-speed, 0.F};
    return Velocity{dx / len * speed, dy / len * speed};
}

} // namespace rtype::server
