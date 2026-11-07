#pragma once

#include <cstdint>

namespace rtype::server {

// How a monster moves. Every enemy type in enemies.json maps onto one of these:
//   Advance: flies left at its speed, forever (basic, tank).
//   Flight:  flies left while bobbing up and down, its bob drifting towards the nearest
//            player (bat). With homing 0 it is a plain sine wave (bydo_wave).
//   Crawl:   walks along the floor or the ceiling, stops dead at stopAtX and shoots from
//            there until killed (slug). With `hold` set, it walks on after that many seconds.
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

// Server-only component: the brain MonsterSystem runs every step. Spawner adds it next to
// the engine's EnemyBehavior (which stays, as the "this is an enemy" tag other code uses).
struct Monster
{
    MonsterKind kind = MonsterKind::Advance;
    float speed = 0.F;        // units per second, leftwards
    bool aimAtPlayer = false; // shots go towards the nearest ship instead of straight left

    // Flight
    float amplitude = 0.F; // half the bob's height
    float frequency = 0.F; // bobs per second
    float homing = 0.F;    // how fast the bob's centre follows the nearest ship, units/s
    float centerY = 0.F;   // set at spawn: the bob's centre, it moves when homing > 0
    float clock = 0.F;

    // Crawl
    Surface surface = Surface::Floor;
    float stopMinX = 0.F; // the slug stops somewhere in [stopMinX, stopMaxX], picked once
    float stopMaxX = 0.F;
    float stopAtX = -1.F; // below 0: not picked yet
    float hold = -1.F;    // seconds spent firing; below 0: stays until killed
    float windup = 0.5F;  // seconds between stopping and the first shot
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
