#pragma once

#include "ecs/Entity.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace rtype::server {

// What players see of a boss: GameSession turns it into a BOSS_STATE message
// for the health bar.
struct BossStatus
{
    engine::Entity body; // the entity the bar belongs to
    std::string name;
    int health = 0;
    int maxHealth = 1;
    std::uint8_t phase = 0; // 0 while it flies in, then 1 .. phaseCount
    std::uint8_t phaseCount = 1;
    std::vector<float> marks; // health fractions where a phase starts (BossDef::phases)
    bool exposed = false;     // its weak point takes damage right now
    bool enraged = false;     // last phase
    bool dying = false;       // health is 0, the death animation is playing
    int score = 0;            // points for the ship that lands the killing blow

    bool operator==(const BossStatus&) const = default;
};

} // namespace rtype::server
