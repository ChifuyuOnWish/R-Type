#include "monsters/MonsterSystem.hpp"

#include "World.hpp"
#include "components/Components.hpp"
#include "ecs/Registry.hpp"
#include "monsters/Monster.hpp"
#include "monsters/Targeting.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace rtype::server {

using namespace engine;

namespace {

constexpr float ScreenMargin = 20.F;
constexpr float KeepTimer = -1.F;

void setWeapon(Registry& registry, Entity e, bool on, float firstShotIn)
{
    if (!registry.hasComponent<Weapon>(e))
        return;
    Weapon& w = registry.getComponent<Weapon>(e);
    if (on && !w.automatic && firstShotIn >= 0.F)
        w.timer = firstShotIn;
    w.automatic = on;
}

void advance(Monster& m, Velocity& v)
{
    v.dx = -m.speed;
    v.dy = 0.F;
}

void flight(Registry& registry, Entity e, const std::vector<Position>& ships, Monster& m,
            const Position& p, float halfH, Velocity& v, float dt)
{
    m.clock += dt;
    const auto ship = nearest(ships, p);
    if (m.homing > 0.F && ship) {
        const float step = m.homing * dt;
        m.centerY += std::clamp(ship->y - m.centerY, -step, step);
    }

    if (m.aimAtPlayer)
        setWeapon(registry, e, ship && ship->x < p.x, KeepTimer);

    const float low = PlayTop + halfH + m.amplitude + ScreenMargin;
    const float high = PlayBottom - halfH - m.amplitude - ScreenMargin;
    m.centerY = low < high ? std::clamp(m.centerY, low, high) : WorldHeight / 2.F;

    const float target =
        m.centerY + m.amplitude * std::sin(2.F * std::numbers::pi_v<float> * m.frequency * m.clock);
    v.dx = -m.speed;

    v.dy = dt > 0.F ? (target - p.y) / dt : 0.F;
}

void crawl(Registry& registry, Entity e, Monster& m, Position& p, Velocity& v, float dt,
           std::mt19937& rng)
{
    if (m.stopAtX < 0.F) {
        std::uniform_real_distribution<float> pick(m.stopMinX, std::max(m.stopMinX, m.stopMaxX));
        m.stopAtX = pick(rng);
    }
    m.phaseTime += dt;
    v.dy = 0.F;
    switch (m.phase) {
    case CrawlPhase::Approach:

        if (p.x - m.speed * dt <= m.stopAtX) {
            v.dx = dt > 0.F ? (m.stopAtX - p.x) / dt : 0.F;
            m.phase = CrawlPhase::Firing;
            m.phaseTime = 0.F;
            setWeapon(registry, e, true, m.windup);
        } else {
            v.dx = -m.speed;
            setWeapon(registry, e, false, 0.F);
        }
        break;
    case CrawlPhase::Firing:

        v.dx = 0.F;
        if (m.hold >= 0.F && m.phaseTime >= m.hold) {
            m.phase = CrawlPhase::Leaving;
            m.phaseTime = 0.F;
            setWeapon(registry, e, false, 0.F);
        }
        break;
    case CrawlPhase::Leaving:
        v.dx = -m.speed;
        break;
    }
}

} // namespace

MonsterSystem::MonsterSystem(std::mt19937& rng) : _rng(rng) {}

void MonsterSystem::update(Registry& registry, float dt)
{
    const std::vector<Position> ships = shipPositions(registry);

    registry.each<Monster, Position, Velocity>([&](Entity e, Monster& m, Position& p, Velocity& v) {
        switch (m.kind) {
        case MonsterKind::Advance:
            advance(m, v);
            break;
        case MonsterKind::Flight: {
            const float halfH = registry.hasComponent<Hitbox>(e)
                                    ? registry.getComponent<Hitbox>(e).height / 2.F
                                    : 0.F;
            flight(registry, e, ships, m, p, halfH, v, dt);
            break;
        }
        case MonsterKind::Crawl:
            crawl(registry, e, m, p, v, dt, _rng);
            break;
        }
    });
}

} // namespace rtype::server
