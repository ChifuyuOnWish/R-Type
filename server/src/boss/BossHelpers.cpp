#include "boss/BossHelpers.hpp"

#include "ServerComponents.hpp"
#include "monsters/Targeting.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace rtype::server::boss {

using namespace engine;

namespace {
constexpr float Radians = std::numbers::pi_v<float> / 180.F;
} // namespace

void announce(GameContext& ctx, Entity shooter)
{
    if (const std::uint32_t source = netIdOf(ctx.registry, shooter))
        ctx.events.push_back(protocol::GameEventMessage{protocol::GameEventType::Shoot, source, 0});
}

int drainArmor(Registry& registry, Entity piece)
{
    if (!registry.isAlive(piece) || !registry.hasComponent<Health>(piece))
        return 0;
    Health& health = registry.getComponent<Health>(piece);
    const int taken = std::max(0, health.max - health.current);
    health.current = health.max;
    return taken;
}

void steer(Registry& registry, Entity piece, Position target, float dt)
{
    if (!registry.isAlive(piece) || dt <= 0.F)
        return;
    const Position& at = registry.getComponent<Position>(piece);
    registry.getComponent<Velocity>(piece) =
        Velocity{(target.x - at.x) / dt, (target.y - at.y) / dt};
}

Velocity heading(float degrees, float speed)
{
    return Velocity{std::cos(degrees * Radians) * speed, std::sin(degrees * Radians) * speed};
}

float degreesTowards(Position from, Position to)
{
    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    if (dx * dx + dy * dy < 1.F)
        return 180.F;
    return std::atan2(dy, dx) / Radians;
}

void fan(GameContext& ctx, Entity shooter, Position from, std::uint16_t type, int count,
         float center, float spread, float speed)
{
    if (count <= 0)
        return;
    const float step = count > 1 ? spread / static_cast<float>(count - 1) : 0.F;
    const float first = count > 1 ? center - spread / 2.F : center;
    for (int i = 0; i < count; ++i)
        ctx.spawner.spawnBossShot(shooter, from,
                                  heading(first + step * static_cast<float>(i), speed), type);
    announce(ctx, shooter);
}

void aimedFan(GameContext& ctx, Entity shooter, Position from, std::uint16_t type, int count,
              float spread, float speed)
{
    const auto target = nearestShip(ctx.registry, from);
    const float center = target ? degreesTowards(from, *target) : 180.F;
    fan(ctx, shooter, from, type, count, center, spread, speed);
}

void ring(GameContext& ctx, Entity shooter, Position from, std::uint16_t type, int count,
          float offset, float speed)
{
    if (count <= 0)
        return;
    const float step = 360.F / static_cast<float>(count);
    for (int i = 0; i < count; ++i)
        ctx.spawner.spawnBossShot(shooter, from,
                                  heading(offset + step * static_cast<float>(i), speed), type);
    announce(ctx, shooter);
}

std::vector<Entity> summon(GameContext& ctx, const level::EnemyDef& enemy, Position anchor,
                           std::span<const level::Vec2> offsets)
{
    std::vector<Entity> spawned;
    for (const level::Vec2& o : offsets)
        spawned.push_back(ctx.spawner.spawnEnemy(enemy, Position{anchor.x + o.x, anchor.y + o.y},
                                                 enemy.health, 1.F));
    return spawned;
}

Tuning::Tuning(std::map<std::string, float> defaults, const std::map<std::string, float>& data)
    : _values(std::move(defaults))
{
    for (const auto& [key, value] : data)
        _values[key] = value;
}

float Tuning::get(const std::string& key) const
{
    const auto it = _values.find(key);
    if (it == _values.end())
        throw std::logic_error("boss tuning: no default for '" + key + "'");
    return it->second;
}

int Tuning::count(const std::string& key) const
{
    return std::max(0, static_cast<int>(std::lround(get(key))));
}

} // namespace rtype::server::boss
