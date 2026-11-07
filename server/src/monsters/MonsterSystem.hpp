#pragma once

#include "ISystem.hpp"

#include <random>

namespace rtype::server {

// Moves every entity that has a Monster, and switches its weapon on and off. Replaces the
// engine's EnemyBehaviorSystem in the pipeline, between ModeUpdate and Weapon:
// it writes Velocity (MotionSystem then moves it) and Weapon::automatic (WeaponSystem fires).
class MonsterSystem : public engine::ISystem
{
public:
    // The session's generator, so a seeded run picks the same slug stop points.
    explicit MonsterSystem(std::mt19937& rng);
    void update(engine::Registry& registry, float dt) override;

private:
    std::mt19937& _rng;
};

} // namespace rtype::server
