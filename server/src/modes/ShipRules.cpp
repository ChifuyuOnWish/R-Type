#include "ShipRules.hpp"

#include "World.hpp"
#include "components/Components.hpp"

namespace rtype::server {

using namespace engine;

namespace {
constexpr float RespawnInvincibility = 2.F;
constexpr float ShipStartX = 150.F;
constexpr float ShipSpacing = 200.F;
} // namespace

Entity spawnShipInLane(GameContext& context, network::PeerId peer)
{
    return context.spawner.spawnPlayerShip(
        peer, Position{ShipStartX, ShipSpacing * static_cast<float>(peer)});
}

void respawnDeadShips(Registry& registry)
{
    registry.each<Controllable, Health, Position>(
        [&registry](Entity ship, Controllable&, Health& health, Position& position) {
            if (health.current > 0)
                return;
            health.current = health.max;
            position = Position{ShipStartX, WorldHeight / 2.F};
            if (registry.hasComponent<Invincibility>(ship))
                registry.getComponent<Invincibility>(ship).remaining = RespawnInvincibility;
        });
}

} // namespace rtype::server
