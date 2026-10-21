#include "InfiniteMode.hpp"

#include "Components.hpp"
#include "World.hpp"

#include <random>

namespace rtype::server {

using namespace engine;

namespace {
constexpr float MinSpawnInterval = 1.F;
constexpr float MaxSpawnInterval = 2.F;
constexpr float SpawnMargin = 80.F;
constexpr float MinFirstShot = 0.5F;
constexpr float MaxFirstShot = 2.5F;
constexpr float RespawnInvincibility = 2.F;
constexpr float ShipStartX = 150.F;
constexpr float ShipSpacing = 200.F;
} // namespace

void InfiniteMode::start(GameContext&)
{
    _spawnTimer = 0.F;
}

void InfiniteMode::update(GameContext& ctx, float dt)
{
    _spawnTimer -= dt;
    if (_spawnTimer > 0.F)
        return;
    std::uniform_real_distribution<float> next(MinSpawnInterval, MaxSpawnInterval);
    std::uniform_real_distribution<float> height(SpawnMargin, WorldHeight - SpawnMargin);
    std::uniform_real_distribution<float> firstShot(MinFirstShot, MaxFirstShot);
    _spawnTimer = next(ctx.rng);
    // 40 units right of the screen: inside BoundsSystem's 100-unit margin, so it isn't removed.
    ctx.spawner.spawnBasicEnemy(Position{WorldWidth + 40.F, height(ctx.rng)}, firstShot(ctx.rng));
}

void InfiniteMode::resolve(GameContext& ctx, float)
{
    Registry& registry = ctx.registry;
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

Entity InfiniteMode::spawnShip(GameContext& ctx, network::PeerId peer)
{
    return ctx.spawner.spawnPlayerShip(
        peer, Position{ShipStartX, ShipSpacing * static_cast<float>(peer)});
}

ModeState InfiniteMode::state() const
{
    return ModeState::Running;
}

} // namespace rtype::server
