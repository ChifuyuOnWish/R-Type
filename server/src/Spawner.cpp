#include "Spawner.hpp"

#include "EntityType.hpp"
#include "ServerComponents.hpp"

namespace rtype::server {

using namespace engine;
using namespace protocol;

namespace {
constexpr float PlayerMissileSpeed = 900.F;
constexpr float EnemyMissileSpeed = 400.F;
constexpr float MissileLifetime = 3.F;
constexpr float EnemySpeed = 150.F;
constexpr float EnemyFireCooldown = 2.5F;

engine::EnemyPattern pattern(level::Movement movement)
{
    switch (movement) {
    case level::Movement::Sine:
        return engine::EnemyPattern::Sine;
    case level::Movement::Follow:
        return engine::EnemyPattern::Chase;
    default:
        return engine::EnemyPattern::Straight;
    }
}

} // namespace

Spawner::Spawner(Registry& registry, EntityFactory& factory, std::vector<GameEventMessage>& events)
    : _registry(registry), _factory(factory), _events(events)
{
}

Entity Spawner::spawnPlayerShip(network::PeerId peer, Position position)
{
    ComponentParams p;
    p.position = position;
    p.health = Health{3, 3};
    p.hitbox = Hitbox{64.F, 32.F};
    p.damage = Damage{1};
    p.weapon = Weapon{0.25F, 0.F, static_cast<unsigned>(EntityType::PlayerMissile), 1.F, false};
    p.playerId = peer;
    p.team = PlayerTeam;
    const Entity ship = _factory.create("player", p);
    _registry.addComponent(ship, NetType{static_cast<std::uint16_t>(playerShipType(peer))});
    return ship;
}

Entity Spawner::spawnProjectile(Registry& registry, Entity shooter, Position from, unsigned type)
{
    const bool fromPlayer = type == static_cast<unsigned>(EntityType::PlayerMissile);
    ComponentParams p;
    p.position = Position{from.x + (fromPlayer ? 40.F : -40.F), from.y};
    p.velocity = Velocity{fromPlayer ? PlayerMissileSpeed : -EnemyMissileSpeed, 0.F};
    p.hitbox = Hitbox{16.F, 8.F};
    p.damage = Damage{1};
    p.lifetime = Lifetime{MissileLifetime};
    p.owner = shooter;
    p.team = fromPlayer ? PlayerTeam : EnemyTeam;
    const Entity missile = _factory.create(fromPlayer ? "player_missile" : "enemy_missile", p);
    registry.addComponent(missile, NetType{static_cast<std::uint16_t>(type)});
    if (const std::uint32_t source = netIdOf(registry, shooter))
        _events.push_back(GameEventMessage{GameEventType::Shoot, source, 0});
    return missile;
}

Entity Spawner::spawnBasicEnemy(Position position, float firstShotDelay)
{
    ComponentParams p;
    p.position = position;
    p.velocity = Velocity{-EnemySpeed, 0.F};
    p.health = Health{1, 1};
    p.damage = Damage{1};
    p.hitbox = Hitbox{64.F, 64.F};
    p.behavior = EnemyBehavior{EnemyPattern::Straight, EnemySpeed, 0.F, 0.F};
    p.weapon = Weapon{EnemyFireCooldown, firstShotDelay,
                      static_cast<unsigned>(EntityType::EnemyMissile), 1.F, true};
    p.team = EnemyTeam;
    const Entity enemy = _factory.create("enemy_basic", p);
    _registry.addComponent(enemy, NetType{static_cast<std::uint16_t>(EntityType::EnemyBasic)});
    return enemy;
}

Entity Spawner::spawnEnemy(const level::EnemyDef& def, Position position, int health,
                           float firstShotDelay)
{
    ComponentParams p;
    p.position = position;
    p.velocity = Velocity{-def.speed, 0.F};
    p.health = Health{health, health};
    p.damage = Damage{def.contactDamage};
    p.hitbox = Hitbox{def.hitbox.x, def.hitbox.y};
    p.behavior = EnemyBehavior{pattern(def.movement), def.speed, def.amplitude, def.frequency};
    if (def.weapon)
        p.weapon =
            Weapon{def.weapon->cooldown, firstShotDelay, def.weapon->projectileNetType, 1.F, true};
    p.team = EnemyTeam;
    const Entity enemy = _factory.create("enemy_basic", p);
    _registry.addComponent(enemy, NetType{def.netType});
    return enemy;
}

} // namespace rtype::server
