#pragma once

#include <cstdint>

namespace rtype::server {

enum class MonsterKind : std::uint8_t
{
    Advance,
    Flight,
    Crawl
};
enum class Surface : std::uint8_t
{
    Floor,
    Ceiling
};
enum class CrawlPhase : std::uint8_t
{
    Approach,
    Firing,
    Leaving
};

struct Monster
{
    MonsterKind kind = MonsterKind::Advance;
    float speed = 0.F;
    bool aimAtPlayer = false;

    float amplitude = 0.F;
    float frequency = 0.F;
    float homing = 0.F;
    float centerY = 0.F;
    float clock = 0.F;

    Surface surface = Surface::Floor;
    float stopMinX = 0.F;
    float stopMaxX = 0.F;
    float stopAtX = -1.F;
    float hold = -1.F;
    float windup = 0.5F;
    CrawlPhase phase = CrawlPhase::Approach;
    float phaseTime = 0.F;

    static Monster advance(float speed)
    {
        Monster m;
        m.kind = MonsterKind::Advance;
        m.speed = speed;
        return m;
    }
};

} // namespace rtype::server
