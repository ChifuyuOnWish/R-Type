#include "EntityFactory.hpp"

#include <stdexcept>
#include <utility>

namespace rtype::engine {

namespace {

Entity buildPlayer(Registry& r, const ComponentParams& p)
{
    const Entity e = r.createEntity();
    r.addComponent(e, p.position);
    r.addComponent(e, p.velocity);
    r.addComponent(e, p.health);
    r.addComponent(e, p.hitbox);
    r.addComponent(e, Controllable{});
    r.addComponent(e, PlayerOwner{p.playerId});
    r.addComponent(e, p.weapon);
    r.addComponent(e, p.score);
    r.addComponent(e, p.invincibility);
    r.addComponent(e, p.damage);
    r.addComponent(e, Team{p.team});
    return e;
}

Entity buildEnemy(Registry& r, const ComponentParams& p, EnemyPattern defaultPattern)
{
    EnemyBehavior behavior;
    if (p.behavior) {
        behavior = *p.behavior;
    } else {
        behavior.pattern = defaultPattern;
    }
    const Entity e = r.createEntity();
    r.addComponent(e, p.position);
    r.addComponent(e, p.velocity);
    r.addComponent(e, p.health);
    r.addComponent(e, p.damage);
    r.addComponent(e, p.hitbox);
    r.addComponent(e, behavior);
    r.addComponent(e, p.weapon);
    r.addComponent(e, p.score);
    r.addComponent(e, Team{p.team});
    return e;
}

Entity buildMissile(Registry& r, const ComponentParams& p)
{
    const Entity e = r.createEntity();
    r.addComponent(e, p.position);
    r.addComponent(e, p.velocity);
    r.addComponent(e, p.damage);
    r.addComponent(e, p.hitbox);
    r.addComponent(e, Projectile{p.owner, p.lifetime.remaining, p.pierceCount});
    r.addComponent(e, p.lifetime);
    r.addComponent(e, Team{p.team});
    return e;
}

Entity buildExplosion(Registry& r, const ComponentParams& p)
{
    const Entity e = r.createEntity();
    r.addComponent(e, p.position);
    r.addComponent(e, p.lifetime);
    return e;
}

Entity buildBackground(Registry& r, const ComponentParams& p)
{
    const Entity e = r.createEntity();
    r.addComponent(e, p.position);
    r.addComponent(e, p.velocity);
    return e;
}

} // namespace

EntityFactory::EntityFactory(Registry& registry) : _registry(registry)
{
    registerArchetype("player", buildPlayer);
    registerArchetype("enemy_basic", [](Registry& r, const ComponentParams& p) {
        return buildEnemy(r, p, EnemyPattern::Straight);
    });
    registerArchetype("enemy_sine", [](Registry& r, const ComponentParams& p) {
        return buildEnemy(r, p, EnemyPattern::Sine);
    });
    registerArchetype("enemy_chase", [](Registry& r, const ComponentParams& p) {
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

Entity EntityFactory::create(const std::string& name, const ComponentParams& params) const
{
    const auto it = _builders.find(name);
    if (it == _builders.end()) {
        throw std::invalid_argument("EntityFactory::create: unknown archetype '" + name + "'");
    }
    return it->second(_registry, params);
}

} // namespace rtype::engine
