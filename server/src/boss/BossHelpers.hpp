#pragma once

#include "GameContext.hpp"
#include "LevelData.hpp"
#include "components/Components.hpp"

#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <vector>

// Building blocks shared by every boss: moving pieces, armour, volleys and
// summons. Angles are in degrees, screen style: 0 = right, 90 = down, 180 =
// straight left.
namespace rtype::server::boss {

// Health given to armoured pieces (body, weak point). They never die by
// themselves: the boss reads what they lost each step with drainArmor() and
// applies it to its own health.
inline constexpr int ArmorHealth = 1'000'000;

// Damage `piece` took since the last call, before any multiplier, and refills
// it. 0 when the piece is gone.
int drainArmor(engine::Registry& registry, engine::Entity piece);

// Sets the velocity that makes MotionSystem land `piece` exactly on `target`
// this step, so clients extrapolate it smoothly. Never teleports.
void steer(engine::Registry& registry, engine::Entity piece, engine::Position target, float dt);

engine::Velocity heading(float degrees, float speed);
// One Shoot GAME_EVENT from `shooter` (clients play the shot sound).
void announce(GameContext& ctx, engine::Entity shooter);
float degreesTowards(engine::Position from, engine::Position to);

// `count` shots spread evenly over `spread` degrees around `center`. One Shoot
// event per volley, not per bullet, so clients don't play 12 shot sounds at
// once.
void fan(GameContext& ctx, engine::Entity shooter, engine::Position from, std::uint16_t type,
         int count, float center, float spread, float speed);
// The same fan, centred on the nearest ship (straight left when nobody is
// alive).
void aimedFan(GameContext& ctx, engine::Entity shooter, engine::Position from, std::uint16_t type,
              int count, float spread, float speed);
// `count` shots all around, the first one at `offset` degrees.
void ring(GameContext& ctx, engine::Entity shooter, engine::Position from, std::uint16_t type,
          int count, float offset, float speed);

// Enemies from enemies.json at anchor + each offset, with their normal brain.
std::vector<engine::Entity> summon(GameContext& ctx, const level::EnemyDef& enemy,
                                   engine::Position anchor, std::span<const level::Vec2> offsets);

// Tuning numbers: the class's defaults overridden by the "tuning" object in
// enemies.json. BossRegistry::check already refused unknown keys, so get() only
// fails on a typo in code.
class Tuning
{
public:
    Tuning() = default;
    Tuning(std::map<std::string, float> defaults, const std::map<std::string, float>& data);
    float get(const std::string& key) const;
    int count(const std::string& key) const; // rounded, at least 0

private:
    std::map<std::string, float> _values;
};

} // namespace rtype::server::boss
