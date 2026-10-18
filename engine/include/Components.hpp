#pragma once

#include "Entity.hpp"

#include <cstdint>

namespace rtype::engine {

struct Position
{
    float x = 0.F;
    float y = 0.F;
};

struct Velocity
{
    float dx = 0.F;
    float dy = 0.F;
};

struct Hitbox
{
    float width = 0.F;
    float height = 0.F;
};

struct Health
{
    int current = 1;
    int max = 1;
};

struct Hit
{
    int amount = 0;
    Entity source{};
};

struct Damage
{
    int value = 0;
};

struct Lifetime
{
    float remaining = 0.F;
};

struct Invincibility
{
    float remaining = 0.F;
};

struct Team
{
    std::uint8_t id = 0;
};

struct Controllable
{
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool shoot = false;
};

struct PlayerOwner
{
    unsigned playerId = 0;
};

enum class EnemyPattern
{
    Straight,
    Sine,
    Chase,
};

struct EnemyBehavior
{
    EnemyPattern pattern = EnemyPattern::Straight;
    float speed = 0.F;
    float amplitude = 0.F;
    float frequency = 0.F;
};

struct Weapon
{
    float cooldown = 0.F;
    float timer = 0.F;
    unsigned projectileType = 0;
    float rate = 1.F;
    bool automatic = false;
};

struct Projectile
{
    Entity owner;
    float lifetime = 0.F;
    unsigned pierceCount = 0;
};

struct Score
{
    int value = 0;
    int total = 0;
};

} // namespace rtype::engine
