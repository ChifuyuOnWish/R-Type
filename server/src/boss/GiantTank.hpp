#pragma once

#include "boss/BossHelpers.hpp"
#include "boss/IBoss.hpp"

#include <array>
#include <cstddef>
#include <map>
#include <string>

namespace rtype::server {

// The Giant Tank: the tank enemy, scaled up into the campaign's last boss. Its
// whole hull takes damage. It has 3 phases (at 65% and 30% health by default)
// and four attacks:
//
//   Fan volley    a spread of shots straight ahead              phase 1
//   Aimed burst   a tight burst at the nearest ship              phases 1, 2, 3
//   Shell hose    a stream of shots sweeping up and down         phase 2
//   Dash          rattles (the tell), charges across the screen  phases 2, 3
//                 and bursts into a ring of shots at the end
//   Reinforcements  rolls out a trio of minions (enemies.json)   every phase
//
// Phase 1 bobs gently and alternates fan and burst. Phase 2 sweeps up and down,
// alternates hose and burst, and dashes. Phase 3 tracks the nearest ship, fires
// bursts faster and dashes more often. Every number is a "tuning" key in
// enemies.json.
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
        Shifting, // between two phases: shakes, takes no damage, doesn't shoot
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
    engine::Entity _weak; // only when enemies.json gives it a weakPoint

    Stage _stage = Stage::Entering;
    int _phase = 0;
    int _phaseCount = 1;
    float _health = 0.F;
    float _maxHealth = 1.F;
    float _stageTime = 0.F;
    float _time = 0.F; // fight clock, drives the bobbing
    engine::Position _start;
    engine::Position _pos; // where the body is this step
    float _homeX = 0.F;
    float _anchorX = 0.F;

    float _volleyTimer = 0.F;
    int _volley = 0;       // alternates the two patterns of the phase
    float _hoseLeft = 0.F; // seconds of hose still to fire
    float _hoseTimer = 0.F;
    float _hoseAngle = 180.F;
    float _hoseTurn = 1.F;
    float _summonTimer = 0.F;
    float _dashTimer = 0.F; // time to the next dash
    Dash _dash = Dash::None;
    float _dashStage = 0.F; // time left in the current dash stage
    float _rumbleTimer = 0.F;
};

} // namespace rtype::server
