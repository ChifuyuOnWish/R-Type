#include "EntityFactory.hpp"

#include <stdexcept>
#include <utility>

namespace rtype::engine {

namespace {

NetType makeNet(EntityType type, const SpawnParams& p)
{
    return NetType{p.netId, type, p.x, p.y};
}

Entity buildPlayer(Registry& r, const SpawnParams& p)
{
    const Entity e = r.createEntity();
    r.addComponent(e, Position{p.x, p.y});
    r.addComponent(e, Velocity{});
    r.addComponent(e, Health{});
    r.addComponent(e, Hitbox{});
    r.addComponent(e, Controllable{});
    r.addComponent(e, PlayerOwner{p.playerId});
    r.addComponent(e, Weapon{});
    r.addComponent(e, Score{});
    r.addComponent(e, Invincibility{});
    r.addComponent(e, makeNet(EntityType::Player, p));
    return e;
}

Entity buildEnemy(Registry& r, const SpawnParams& p, EnemyPattern pattern)
{
    const Entity e = r.createEntity();
    EnemyBehavior behavior;
    behavior.pattern = pattern;

    r.addComponent(e, Position{p.x, p.y});
    r.addComponent(e, Velocity{});
    r.addComponent(e, Health{});
    r.addComponent(e, Damage{});
    r.addComponent(e, Hitbox{});
    r.addComponent(e, behavior);
    r.addComponent(e, Weapon{});
    r.addComponent(e, Score{});
    r.addComponent(e, makeNet(EntityType::Enemy, p));
    return e;
}

Entity buildMissile(Registry& r, const SpawnParams& p)
{
    const Entity e = r.createEntity();
    r.addComponent(e, Position{p.x, p.y});
    r.addComponent(e, Velocity{});
    r.addComponent(e, Damage{});
    r.addComponent(e, Hitbox{});
    Projectile projectile;
    projectile.owner = p.owner;
    r.addComponent(e, projectile);
    r.addComponent(e, Lifetime{});
    return e;
}

Entity buildExplosion(Registry& r, const SpawnParams& p)
{
    const Entity e = r.createEntity();
    r.addComponent(e, Position{p.x, p.y});
    r.addComponent(e, makeNet(EntityType::Effect, p));
    r.addComponent(e, Lifetime{});
    return e;
}

Entity buildBackground(Registry& r, const SpawnParams& p)
{
    const Entity e = r.createEntity();
    r.addComponent(e, Position{p.x, p.y});
    r.addComponent(e, Velocity{});
    r.addComponent(e, makeNet(EntityType::Decor, p));
    return e;
}

} // namespace

EntityFactory::EntityFactory(Registry& registry) : _registry(registry)
{
    registerArchetype("player", buildPlayer);
    registerArchetype("enemy_basic", [](Registry& r, const SpawnParams& p) {
        return buildEnemy(r, p, EnemyPattern::Straight);
    });
    registerArchetype("enemy_sine", [](Registry& r, const SpawnParams& p) {
        return buildEnemy(r, p, EnemyPattern::Sine);
    });
    registerArchetype("enemy_chase", [](Registry& r, const SpawnParams& p) {
        return buildEnemy(r, p, EnemyPattern::Chase);
    });
    registerArchetype("player_missile", buildMissile);
    registerArchetype("enemy_missile", buildMissile);
    registerArchetype("explosion", buildExplosion);
    registerArchetype("background", buildBackground);
}

void EntityFactory::registerArchetype(std::string name, Builder builder)
{
    _builders[std::move(name)] = std::move(builder);
}

bool EntityFactory::has(const std::string& name) const
{
    return _builders.find(name) != _builders.end();
}

Entity EntityFactory::create(const std::string& name, const SpawnParams& params) const
{
    const auto it = _builders.find(name);
    if (it == _builders.end()) {
        throw std::invalid_argument("EntityFactory::create: unknown archetype '" + name + "'");
    }
    return it->second(_registry, params);
}

} // namespace rtype::engine
