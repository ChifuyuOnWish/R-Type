#include "Spawner.hpp"

#include "EntityType.hpp"
#include "ServerComponents.hpp"
#include "World.hpp"
#include "monsters/Monster.hpp"
#include "monsters/Targeting.hpp"
#include "terrain/Tile.hpp"

namespace rtype::server {

using namespace engine;
using namespace protocol;

namespace {
constexpr float PlayerMissileSpeed = 900.F;
constexpr float EnemyMissileSpeed = 400.F;
constexpr float MissileLifetime = 3.F;
constexpr float MuzzleOffset = 40.F;
constexpr float EnemySpeed = 150.F;
constexpr float EnemyFireCooldown = 2.5F;
constexpr float BossShotLifetime = 8.F;
constexpr float BossShotSize = 24.F;

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

Monster brain(const level::EnemyDef& def, Position position)
{
    Monster m = Monster::advance(def.speed);
    m.aimAtPlayer = def.weapon && def.weapon->aim == level::Aim::NearestPlayer;
    switch (def.movement) {
    case level::Movement::Sine:
    case level::Movement::Flight:
        m.kind = MonsterKind::Flight;
        m.amplitude = def.amplitude;
        m.frequency = def.frequency;
        m.homing = def.homing;
        m.centerY = position.y;
        break;
    case level::Movement::Follow:
        m.kind = MonsterKind::Flight;
        m.homing = def.speed;
        m.centerY = position.y;
        break;
    case level::Movement::Crawl:
        m.kind = MonsterKind::Crawl;
        m.surface = def.onCeiling ? Surface::Ceiling : Surface::Floor;
        m.stopMinX = def.stopMinX;
        m.stopMaxX = def.stopMaxX;
        m.hold = def.hold;
        break;
    default:
        break;
    }
    return m;
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
    const float speed = fromPlayer ? PlayerMissileSpeed : EnemyMissileSpeed;
    Velocity velocity{fromPlayer ? speed : -speed, 0.F};
    const bool aimed = !fromPlayer && registry.hasComponent<Monster>(shooter) &&
                       registry.getComponent<Monster>(shooter).aimAtPlayer;
    if (aimed) {
        if (const auto target = nearestShip(registry, from))
            velocity = towards(from, *target, speed);
    }
    ComponentParams p;
    p.position = Position{from.x + velocity.dx / speed * MuzzleOffset,
                          from.y + velocity.dy / speed * MuzzleOffset};
    p.velocity = velocity;
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

Entity Spawner::spawnInfiniteEnemy(Position position, bool bat, float firstShotDelay)
{
    ComponentParams p;
    p.position = position;
    const float speed = bat ? 180.0F : EnemySpeed;
    const int health = bat ? 2 : 1;
    p.velocity = Velocity{-speed, 0.F};
    p.health = Health{health, health};
    p.damage = Damage{1};
    p.hitbox = Hitbox{64.F, bat ? 48.0F : 64.0F};
    p.behavior = EnemyBehavior{bat ? EnemyPattern::Sine : EnemyPattern::Straight, speed,
                               bat ? 120.0F : 0.0F, bat ? 0.8F : 0.0F};
    if (bat)
        p.weapon = Weapon{EnemyFireCooldown, firstShotDelay,
                          static_cast<unsigned>(EntityType::EnemyMissile), 1.F, true};
    p.team = EnemyTeam;
    const Entity enemy = _factory.create("enemy_basic", p);
    _registry.addComponent(enemy, NetType{static_cast<std::uint16_t>(
                                      bat ? EntityType::EnemyBat : EntityType::EnemyBasic)});
    Monster mind = Monster::advance(speed);
    if (bat) {
        mind.kind = MonsterKind::Flight;
        mind.amplitude = 120.F;
        mind.frequency = 0.8F;
        mind.homing = 60.F;
        mind.centerY = position.y;
        mind.aimAtPlayer = true;
    }
    _registry.addComponent(enemy, mind);
    return enemy;
}

Entity Spawner::spawnEnemy(const level::EnemyDef& def, Position position, int health,
                           float firstShotDelay)
{
    const bool crawler = def.movement == level::Movement::Crawl;
    if (crawler) {
        const float half = def.hitbox.y / 2.F;
        position.y = def.onCeiling ? PlayTop + half : PlayBottom - half;
    }

    ComponentParams p;
    p.position = position;
    p.velocity = Velocity{-def.speed, 0.F};
    p.health = Health{health, health};
    p.damage = Damage{def.contactDamage};
    p.hitbox = Hitbox{def.hitbox.x, def.hitbox.y};
    p.behavior = EnemyBehavior{pattern(def.movement), def.speed, def.amplitude, def.frequency};
    if (def.weapon)
        p.weapon = Weapon{def.weapon->cooldown, firstShotDelay, def.weapon->projectileNetType, 1.F,
                          !crawler};
    p.team = EnemyTeam;
    const Entity enemy = _factory.create("enemy_basic", p);
    _registry.addComponent(enemy, NetType{def.netType});
    _registry.addComponent(enemy, brain(def, position));
    return enemy;
}

Entity Spawner::spawnTile(const level::TileDef& def, Position center, float size, float scrollSpeed)
{
    const Entity tile = _registry.createEntity();
    _registry.addComponent(tile, center);
    _registry.addComponent(tile, Velocity{-scrollSpeed, 0.F});
    _registry.addComponent(tile, Tile{size, def.solid, def.contactDamage});
    if (def.health)
        _registry.addComponent(tile, Health{*def.health, *def.health});
    _registry.addComponent(tile, NetType{def.netType});
    return tile;
}

Entity Spawner::spawnBossPiece(const BossPieceSpec& spec)
{
    const Entity piece = _registry.createEntity();
    _registry.addComponent(piece, spec.position);
    _registry.addComponent(piece, Velocity{0.F, 0.F});
    _registry.addComponent(piece, Hitbox{spec.hitbox.x, spec.hitbox.y});
    _registry.addComponent(piece, Health{spec.health, spec.health});
    _registry.addComponent(piece, Damage{spec.contactDamage});
    _registry.addComponent(piece, Team{EnemyTeam});
    if (spec.netType != 0)
        _registry.addComponent(piece, NetType{spec.netType});
    return piece;
}

Entity Spawner::spawnBossShot(Entity shooter, Position from, Velocity velocity,
                              std::uint16_t netType)
{
    ComponentParams p;
    p.position = from;
    p.velocity = velocity;
    p.hitbox = Hitbox{BossShotSize, BossShotSize};
    p.damage = Damage{1};
    p.lifetime = Lifetime{BossShotLifetime};
    p.owner = shooter;
    p.team = EnemyTeam;
    const Entity shot = _factory.create("enemy_missile", p);
    _registry.addComponent(shot, NetType{netType});
    return shot;
}

} // namespace rtype::server
