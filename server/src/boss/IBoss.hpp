#pragma once

#include "GameContext.hpp"
#include "LevelData.hpp"
#include "boss/BossStatus.hpp"

namespace rtype::server {

// A boss is a controller that owns several entities (body, weak point,
// drones...), not an entity itself. Damage still goes through the engine's
// Collision and Damage systems; the boss reads what its pieces took on its next
// update. LevelMode owns one and calls it every step.
class IBoss
{
public:
    virtual ~IBoss() = default;

    // Creates its entities just off the right edge. `health` is def.health
    // already scaled for the number of players.
    virtual void spawn(GameContext& ctx, const level::BossDef& def, int health) = 0;
    // Spawn slot of the pipeline (ModeUpdate): moves its pieces, attacks, changes
    // phase.
    virtual void update(GameContext& ctx, float dt) = 0;
    // True once it is dead and its death animation is over: the level is cleared.
    virtual bool defeated() const = 0;
    virtual BossStatus status() const = 0;
};

} // namespace rtype::server
