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

// One piece of a boss: body, weak point, drone. Armoured pieces get boss::ArmorHealth.
struct BossPieceSpec
{
    std::uint16_t netType = 0; // 0: not replicated (an invisible hitbox)
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

    // A boss piece: Position, Velocity, Hitbox, Health, contact Damage, the enemy Team. No
    // Monster brain and no EnemyBehavior: the boss class moves it.
    engine::Entity spawnBossPiece(const BossPieceSpec& spec);
    // An enemy shot with a given velocity (fans, rings). Unlike spawnProjectile it sends no
    // Shoot event: the boss helpers send one per volley.
    engine::Entity spawnBossShot(engine::Entity shooter, engine::Position from,
                                 engine::Velocity velocity, std::uint16_t netType);

private:
    engine::Registry& _registry;
    engine::EntityFactory& _factory;
    std::vector<protocol::GameEventMessage>& _events;
};

} // namespace rtype::server
