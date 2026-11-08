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

struct BossPieceSpec
{
    std::uint16_t netType = 0;
    engine::Position position;
    level::Vec2 hitbox;
    int health = 1;
    int contactDamage = 1;
};

class Spawner
{
public:
    Spawner(engine::Registry& registry, engine::EntityFactory& factory,
            std::vector<protocol::GameEventMessage>& events);

    engine::Entity spawnPlayerShip(network::PeerId peer, engine::Position position);

    engine::Entity spawnProjectile(engine::Registry& registry, engine::Entity shooter,
                                   engine::Position from, unsigned type);
    engine::Entity spawnInfiniteEnemy(engine::Position position, bool bat, float firstShotDelay);

    engine::Entity spawnEnemy(const level::EnemyDef& def, engine::Position position, int health,
                              float firstShotDelay);

    engine::Entity spawnTile(const level::TileDef& def, engine::Position center, float size,
                             float scrollSpeed);

    engine::Entity spawnBossPiece(const BossPieceSpec& spec);
    engine::Entity spawnBossShot(engine::Entity shooter, engine::Position from,
                                 engine::Velocity velocity, std::uint16_t netType);

private:
    engine::Registry& _registry;
    engine::EntityFactory& _factory;
    std::vector<protocol::GameEventMessage>& _events;
};

} // namespace rtype::server
