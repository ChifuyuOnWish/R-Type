#pragma once

#include "boss/BossHelpers.hpp"
#include "boss/IBoss.hpp"

#include <array>
#include <cstddef>
#include <map>
#include <string>

namespace rtype::server {

class GiantTank final : public IBoss
{
public:
    static constexpr std::size_t MaxPhases = 3;
    static constexpr std::array<const char*, 0> Parts{};
    static constexpr std::array<const char*, 1> Minions{"minion"};
    static std::map<std::string, float> Defaults();

    void spawn(GameContext& ctx, const level::BossDef& def, int health) override;
    void update(GameContext& ctx, float dt) override;
    bool defeated() const override;
    BossStatus status() const override;

private:
    enum class Stage
    {
        Entering,
        Fighting,
        Shifting,
        Dying,
        Done
    };
    enum class Dash
    {
        None,
        Windup,
        Charge,
        Hold,
        Return
    };

    void takeDamage(GameContext& ctx);
    void startPhase();
    void fight(GameContext& ctx, float dt);
    void move(GameContext& ctx, float dt);
    void shoot(GameContext& ctx, float dt);
    void dash(GameContext& ctx, float dt);
    void dying(GameContext& ctx, float dt);
    engine::Position muzzle() const;

    const level::BossDef* _def = nullptr;
    boss::Tuning _t;
    engine::Entity _body;
    engine::Entity _weak;

    Stage _stage = Stage::Entering;
    int _phase = 0;
    int _phaseCount = 1;
    float _health = 0.F;
    float _maxHealth = 1.F;
    float _stageTime = 0.F;
    float _time = 0.F;
    engine::Position _start;
    engine::Position _pos;
    float _homeX = 0.F;
    float _anchorX = 0.F;

    float _volleyTimer = 0.F;
    int _volley = 0;
    float _hoseLeft = 0.F;
    float _hoseTimer = 0.F;
    float _hoseAngle = 180.F;
    float _hoseTurn = 1.F;
    float _summonTimer = 0.F;
    float _dashTimer = 0.F;
    Dash _dash = Dash::None;
    float _dashStage = 0.F;
    float _rumbleTimer = 0.F;
};

} // namespace rtype::server
