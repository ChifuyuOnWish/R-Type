#include "GameSystems.hpp"

#include "EntityType.hpp"
#include "Registry.hpp"

#include <utility>

namespace rtype::server {

using namespace engine;

namespace {
constexpr float EnemySpeed = 150.F;
constexpr float EnemyFireCooldown = 2.5F;
constexpr float RespawnInvincibility = 2.F;
} // namespace

WaveSystem::WaveSystem(const EntityFactory& factory, OnSpawn onSpawn)
    : _factory(factory), _onSpawn(std::move(onSpawn))
{
}

void WaveSystem::update(Registry&, float dt)
{
    _timer -= dt;
    if (_timer > 0.F)
        return;
    std::uniform_real_distribution<float> next(1.F, 2.F);
    std::uniform_real_distribution<float> height(80.F, WorldHeight - 80.F);
    std::uniform_real_distribution<float> firstShot(0.5F, EnemyFireCooldown);
    _timer = next(_rng);

    ComponentParams p;
    p.position = Position{WorldWidth + 40.F, height(_rng)};
    p.velocity = Velocity{-EnemySpeed, 0.F};
    p.health = Health{1, 1};
    p.damage = Damage{1};
    p.hitbox = Hitbox{64.F, 64.F};
    p.behavior = EnemyBehavior{EnemyPattern::Straight, EnemySpeed, 0.F, 0.F};
    p.weapon = Weapon{EnemyFireCooldown, firstShot(_rng),
                      static_cast<unsigned>(protocol::EntityType::EnemyMissile), 1.F, true};
    p.team = 1;
    _onSpawn(_factory.create("enemy_basic", p));
}

void RespawnSystem::update(Registry& registry, float)
{
    registry.each<Controllable, Health, Position>(
        [&registry](Entity ship, Controllable&, Health& health, Position& position) {
            if (health.current > 0)
                return;
            health.current = health.max;
            position = Position{150.F, WorldHeight / 2.F};
            if (registry.hasComponent<Invincibility>(ship))
                registry.getComponent<Invincibility>(ship).remaining = RespawnInvincibility;
        });
}

} // namespace rtype::server
