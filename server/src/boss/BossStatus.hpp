#pragma once

#include "ecs/Entity.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace rtype::server {

struct BossStatus
{
    engine::Entity body;
    std::string name;
    int health = 0;
    int maxHealth = 1;
    std::uint8_t phase = 0;
    std::uint8_t phaseCount = 1;
    std::vector<float> marks;
    bool exposed = false;
    bool enraged = false;
    bool dying = false;
    int score = 0;

    bool operator==(const BossStatus&) const = default;
};

} // namespace rtype::server
