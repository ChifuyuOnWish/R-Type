#include "InfiniteMode.hpp"

#include "World.hpp"
#include "components/Components.hpp"
#include "modes/ShipRules.hpp"

#include <random>

namespace rtype::server {

using namespace engine;

namespace {
constexpr float MinSpawnInterval = 1.F;
constexpr float MaxSpawnInterval = 2.F;
constexpr float SpawnMargin = 80.F;
constexpr float MinFirstShot = 0.5F;
constexpr float MaxFirstShot = 2.5F;
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
    ctx.spawner.spawnBasicEnemy(Position{WorldWidth + 40.F, height(ctx.rng)}, firstShot(ctx.rng));
}

void InfiniteMode::resolve(GameContext& ctx, float)
{
    respawnDeadShips(ctx.registry);
}

Entity InfiniteMode::spawnShip(GameContext& ctx, network::PeerId peer)
{
    return spawnShipInLane(ctx, peer);
}

ModeState InfiniteMode::state() const
{
    return ModeState::Running;
}

} // namespace rtype::server
