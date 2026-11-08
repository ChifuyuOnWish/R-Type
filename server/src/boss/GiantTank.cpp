#include "boss/GiantTank.hpp"

#include "ServerComponents.hpp"
#include "World.hpp"
#include "monsters/Targeting.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>

namespace rtype::server {

using namespace engine;

namespace {
constexpr float Tau = 2.F * std::numbers::pi_v<float>;
constexpr float CenterY = WorldHeight / 2.F;
constexpr float WindupTime = 0.8F;
constexpr float ChargeTime = 0.7F;
constexpr float HoldTime = 0.5F;
constexpr float ReturnTime = 1.4F;
constexpr float RumbleEvery = 0.35F; // an explosion sound while it dies

float smooth(float t)
{
    t = std::clamp(t, 0.F, 1.F);
    return t * t * (3.F - 2.F * t);
}

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

// Reinforcements roll out above, in front of and below the gun.
constexpr std::array<level::Vec2, 3> MinionSlots{{{40.F, -150.F}, {-40.F, 0.F}, {40.F, 150.F}}};
} // namespace

std::map<std::string, float> GiantTank::Defaults()
{
    return {
        {"shotSpeed", 340.F},      // units/s
        {"bobAmplitude", 140.F},   // phase 1: up and down around the centre...
        {"bobPeriod", 6.F},        // ...once every 6 s
        {"sweepAmplitude", 280.F}, // phases 2: up and down almost the whole screen...
        {"sweepPeriod", 5.F},
        {"hoverSpeed", 170.F}, // phase 3: how fast it follows the nearest ship
        {"volleyEvery", 1.8F}, // seconds between two attacks (phase 1; x0.8 later)
        {"fanCount", 5.F},
        {"fanSpread", 50.F}, // degrees
        {"burstCount", 3.F},
        {"burstSpread", 16.F},
        {"hoseTime", 1.6F},    // phase 2: seconds a hose lasts
        {"hoseRate", 0.1F},    // seconds between two shots of the hose
        {"hoseTurn", 9.F},     // degrees it turns per shot
        {"hoseArc", 55.F},     // it sweeps 180 +/- this
        {"dashEvery", 7.F},    // phase 2; phase 3 uses 60% of it
        {"dashX", 700.F},      // how far left a dash goes
        {"ringCount", 12.F},   // shots in the ring at the end of a dash
        {"summonEvery", 10.F}, // seconds between two trios of minions
        {"transition", 1.6F},  // seconds of shaking between two phases
        {"deathTime", 2.5F},
    };
}

void GiantTank::spawn(GameContext& ctx, const level::BossDef& def, int health)
{
    _def = &def;
    _t = boss::Tuning(Defaults(), def.tuning);
    _health = static_cast<float>(health);
    _maxHealth = _health;
    _phaseCount = static_cast<int>(def.phases.size()) + 1;
    _homeX = def.entry.stopAtX;

    if (def.entry.from == "top")
        _start = Position{_homeX, PlayTop - 30.F};
    else if (def.entry.from == "bottom")
        _start = Position{_homeX, PlayBottom + 30.F};
    else // inside BoundsSystem's 100-unit margin, or it would remove the body at
         // once
        _start = Position{WorldWidth + 60.F, CenterY};
    _pos = _start;

    _body = ctx.spawner.spawnBossPiece(
        BossPieceSpec{def.netType, _pos, def.hitbox, boss::ArmorHealth, def.contactDamage});
    if (def.weakPoint) {
        const level::WeakPoint& w = *def.weakPoint;
        _weak = ctx.spawner.spawnBossPiece(
            BossPieceSpec{w.netType, Position{_pos.x + w.offset.x, _pos.y + w.offset.y}, w.hitbox,
                          boss::ArmorHealth, def.contactDamage});
    }
    _stage = Stage::Entering;
    _stageTime = 0.F;
}

bool GiantTank::defeated() const
{
    return _stage == Stage::Done;
}

BossStatus GiantTank::status() const
{
    BossStatus s;
    s.body = _body;
    s.name = _def ? _def->name : "";
    s.health = static_cast<int>(std::ceil(std::max(_health, 0.F)));
    s.maxHealth = static_cast<int>(_maxHealth);
    s.phase = static_cast<std::uint8_t>(_stage == Stage::Entering ? 0 : _phase);
    s.phaseCount = static_cast<std::uint8_t>(_phaseCount);
    if (_def)
        s.marks = _def->phases;
    s.exposed = _stage == Stage::Fighting;
    s.enraged = _phase >= 3;
    s.dying = _stage == Stage::Dying || _stage == Stage::Done;
    s.score = _def ? _def->score : 0;
    return s;
}

// The gun: the front (left) edge of the hull, at mid height.
Position GiantTank::muzzle() const
{
    return Position{_pos.x - _def->hitbox.x / 2.F, _pos.y};
}

void GiantTank::update(GameContext& ctx, float dt)
{
    if (_stage == Stage::Done)
        return;
    _stageTime += dt;
    takeDamage(ctx);

    switch (_stage) {
    case Stage::Entering: {
        const float duration = std::max(_def->entry.duration, 0.01F);
        const float k = smooth(_stageTime / duration);
        _pos = Position{lerp(_start.x, _homeX, k), lerp(_start.y, CenterY, k)};
        if (_stageTime >= duration) {
            _phase = 1;
            startPhase();
        }
        break;
    }
    case Stage::Fighting:
        fight(ctx, dt);
        break;
    case Stage::Shifting:
        // Shudders in place: the tell that the next phase is coming.
        _pos.x = _anchorX + 9.F * std::sin(_stageTime * 55.F);
        if (_stageTime >= _t.get("transition")) {
            ++_phase;
            startPhase();
        }
        break;
    case Stage::Dying:
        dying(ctx, dt);
        break;
    case Stage::Done:
        break;
    }
    if (_stage == Stage::Done)
        return;
    boss::steer(ctx.registry, _body, _pos, dt);
    if (_def->weakPoint)
        boss::steer(
            ctx.registry, _weak,
            Position{_pos.x + _def->weakPoint->offset.x, _pos.y + _def->weakPoint->offset.y}, dt);
}

void GiantTank::takeDamage(GameContext& ctx)
{
    const int onBody = boss::drainArmor(ctx.registry, _body);
    const int onWeak = boss::drainArmor(ctx.registry, _weak);
    if (_stage != Stage::Fighting)
        return; // flying in, changing phase or dying: hits bounce off
    const float weak = _def->weakPoint ? _def->weakPoint->damageMultiplier : 1.F;
    _health -=
        static_cast<float>(onBody) * _def->bodyDamageMultiplier + static_cast<float>(onWeak) * weak;

    if (_health <= 0.F) {
        _health = 0.F;
        _stage = Stage::Dying;
        _stageTime = 0.F;
        _anchorX = _pos.x;
        // Its shots still flying go out with it: the win shouldn't kill you.
        std::vector<Entity> shots;
        ctx.registry.each<Projectile>([&](Entity shot, Projectile& p) {
            if (p.owner == _body || p.owner == _weak)
                shots.push_back(shot);
        });
        for (const Entity e : shots)
            ctx.registry.destroyLater(e);
        return;
    }
    if (_phase < _phaseCount &&
        _health <= _maxHealth * _def->phases[static_cast<std::size_t>(_phase - 1)]) {
        _stage = Stage::Shifting;
        _stageTime = 0.F;
        _anchorX = _pos.x;
        _dash = Dash::None;
        _hoseLeft = 0.F;
    }
}

void GiantTank::startPhase()
{
    _stage = Stage::Fighting;
    _stageTime = 0.F;
    _pos.x = _homeX;
    _volleyTimer = 1.F;
    _volley = 0;
    _summonTimer = _t.get("summonEvery") / 2.F;
    _dash = Dash::None;
    _dashTimer = _t.get("dashEvery") * (_phase >= 3 ? 0.6F : 1.F) / 2.F;
}

void GiantTank::fight(GameContext& ctx, float dt)
{
    _time += dt;
    if (_dash != Dash::None) {
        dash(ctx, dt);
        return;
    }
    move(ctx, dt);
    shoot(ctx, dt);

    _summonTimer -= dt;
    if (_summonTimer <= 0.F) {
        _summonTimer = _t.get("summonEvery");
        boss::summon(ctx, *_def->minions.at("minion"), muzzle(), MinionSlots);
    }
    if (_phase >= 2) {
        _dashTimer -= dt;
        if (_dashTimer <= 0.F && _hoseLeft <= 0.F) {
            _dash = Dash::Windup;
            _dashStage = WindupTime;
            _anchorX = _pos.x;
        }
    }
}

void GiantTank::move(GameContext& ctx, float dt)
{
    _pos.x = _homeX;
    if (_phase == 1) {
        _pos.y = CenterY + _t.get("bobAmplitude") * std::sin(Tau * _time / _t.get("bobPeriod"));
    } else if (_phase == 2) {
        _pos.y = CenterY + _t.get("sweepAmplitude") * std::sin(Tau * _time / _t.get("sweepPeriod"));
    } else if (const auto ship = nearestShip(ctx.registry, _pos)) {
        const float step = _t.get("hoverSpeed") * dt;
        _pos.y += std::clamp(ship->y - _pos.y, -step, step);
    }
    const float half = _def->hitbox.y / 2.F;
    _pos.y = std::clamp(_pos.y, PlayTop + half, PlayBottom - half);
}

void GiantTank::shoot(GameContext& ctx, float dt)
{
    const float speed = _t.get("shotSpeed");
    const std::uint16_t type = _def->projectileNetType;

    if (_hoseLeft > 0.F) {
        // A stream of shots sweeping up and down in front of it, like a hose.
        _hoseLeft -= dt;
        _hoseTimer -= dt;
        const float arc = _t.get("hoseArc");
        while (_hoseTimer <= 0.F) {
            ctx.spawner.spawnBossShot(_body, muzzle(), boss::heading(_hoseAngle, speed * 0.9F),
                                      type);
            _hoseAngle += _t.get("hoseTurn") * _hoseTurn;
            if (_hoseAngle > 180.F + arc || _hoseAngle < 180.F - arc)
                _hoseTurn = -_hoseTurn;
            _hoseTimer += _t.get("hoseRate");
        }
        return;
    }

    _volleyTimer -= dt;
    if (_volleyTimer > 0.F)
        return;
    _volleyTimer = _t.get("volleyEvery") * (_phase >= 2 ? 0.8F : 1.F);
    const bool first = _volley++ % 2 == 0;
    if (_phase == 1 && first) {
        boss::fan(ctx, _body, muzzle(), type, _t.count("fanCount"), 180.F, _t.get("fanSpread"),
                  speed);
    } else if (_phase == 2 && first) {
        _hoseLeft = _t.get("hoseTime");
        _hoseTimer = 0.F;
        _hoseAngle = 180.F - _t.get("hoseArc");
        _hoseTurn = 1.F;
        boss::announce(ctx, _body);
    } else {
        boss::aimedFan(ctx, _body, muzzle(), type, _t.count("burstCount"), _t.get("burstSpread"),
                       speed * 1.2F);
    }
}

void GiantTank::dash(GameContext& ctx, float dt)
{
    _dashStage -= dt;
    switch (_dash) {
    case Dash::Windup: // the tell: it rattles before it charges
        _pos.x = _anchorX + 12.F * std::sin(_dashStage * 70.F);
        if (_dashStage <= 0.F) {
            _dash = Dash::Charge;
            _dashStage = ChargeTime;
        }
        break;
    case Dash::Charge: {
        const float k = 1.F - std::max(_dashStage, 0.F) / ChargeTime;
        _pos.x = lerp(_homeX, _t.get("dashX"), k * k); // accelerates
        if (_dashStage <= 0.F) {
            _dash = Dash::Hold;
            _dashStage = HoldTime;
            boss::ring(ctx, _body, _pos, _def->projectileNetType, _t.count("ringCount"),
                       _time * 37.F, _t.get("shotSpeed"));
        }
        break;
    }
    case Dash::Hold:
        _pos.x = _t.get("dashX");
        if (_dashStage <= 0.F) {
            _dash = Dash::Return;
            _dashStage = ReturnTime;
        }
        break;
    case Dash::Return: {
        const float k = smooth(1.F - std::max(_dashStage, 0.F) / ReturnTime);
        _pos.x = lerp(_t.get("dashX"), _homeX, k);
        if (_dashStage <= 0.F) {
            _dash = Dash::None;
            _dashTimer = _t.get("dashEvery") * (_phase >= 3 ? 0.6F : 1.F);
        }
        break;
    }
    case Dash::None:
        break;
    }
}

void GiantTank::dying(GameContext& ctx, float dt)
{
    // Sinks, shaking, with a rumble of explosions, then goes out.
    _pos.x = _anchorX + 6.F * std::sin(_stageTime * 45.F);
    _pos.y += 35.F * dt;
    _rumbleTimer -= dt;
    const std::uint32_t body = netIdOf(ctx.registry, _body);
    if (_rumbleTimer <= 0.F && body != 0) {
        _rumbleTimer = RumbleEvery;
        ctx.events.push_back(protocol::GameEventMessage{protocol::GameEventType::Hit, 0, body});
    }
    if (_stageTime < _t.get("deathTime"))
        return;
    if (body != 0)
        ctx.events.push_back(protocol::GameEventMessage{protocol::GameEventType::Kill, 0, body});
    for (const Entity e : {_body, _weak})
        if (ctx.registry.isAlive(e))
            ctx.registry.destroyLater(e);
    _stage = Stage::Done;
}

} // namespace rtype::server
