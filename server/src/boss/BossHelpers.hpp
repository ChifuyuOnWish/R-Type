#pragma once

#include "GameContext.hpp"
#include "LevelData.hpp"
#include "components/Components.hpp"

#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <vector>

namespace rtype::server::boss {

inline constexpr int ArmorHealth = 1'000'000;

int drainArmor(engine::Registry& registry, engine::Entity piece);
void steer(engine::Registry& registry, engine::Entity piece, engine::Position target, float dt);

engine::Velocity heading(float degrees, float speed);
void announce(GameContext& ctx, engine::Entity shooter);
float degreesTowards(engine::Position from, engine::Position to);

void fan(GameContext& ctx, engine::Entity shooter, engine::Position from, std::uint16_t type,
         int count, float center, float spread, float speed);
void aimedFan(GameContext& ctx, engine::Entity shooter, engine::Position from, std::uint16_t type,
              int count, float spread, float speed);
void ring(GameContext& ctx, engine::Entity shooter, engine::Position from, std::uint16_t type,
          int count, float offset, float speed);

std::vector<engine::Entity> summon(GameContext& ctx, const level::EnemyDef& enemy,
                                   engine::Position anchor, std::span<const level::Vec2> offsets);

class Tuning
{
public:
    Tuning() = default;
    Tuning(std::map<std::string, float> defaults, const std::map<std::string, float>& data);
    float get(const std::string& key) const;
    int count(const std::string& key) const;

private:
    std::map<std::string, float> _values;
};

} // namespace rtype::server::boss
