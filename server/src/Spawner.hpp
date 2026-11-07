#pragma once

#include "LevelData.hpp"
#include "Messages.hpp"
#include "NetworkEvent.hpp"
#include "components/Components.hpp"
#include "ecs/EntityFactory.hpp"
#include "ecs/Registry.hpp"

#include <cstdint>
#include <vector>

namespace rtype::server {

inline constexpr std::uint8_t PlayerTeam = 0;
inline constexpr std::uint8_t EnemyTeam = 1;

class Spawner
{
public:
    Spawner(engine::Registry& registry, engine::EntityFactory& factory,
            std::vector<protocol::GameEventMessage>& events);

    engine::Entity spawnPlayerShip(network::PeerId peer, engine::Position position);
    // A shooter whose Monster has aimAtPlayer fires at the nearest ship, not straight ahead.
    engine::Entity spawnProjectile(engine::Registry& registry, engine::Entity shooter,
                                   engine::Position from, unsigned type);
    engine::Entity spawnInfiniteEnemy(engine::Position position, bool bat, float firstShotDelay);
    // With its Monster brain. A crawler ignores position.y: it goes on its floor or ceiling.
    engine::Entity spawnEnemy(const level::EnemyDef& def, engine::Position position, int health,
                              float firstShotDelay);

private:
    engine::Registry& _registry;
    engine::EntityFactory& _factory;
    std::vector<protocol::GameEventMessage>& _events;
};

} // namespace rtype::server
