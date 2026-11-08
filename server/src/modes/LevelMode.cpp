#include "LevelMode.hpp"

#include "ModeFactory.hpp"
#include "ShipRules.hpp"
#include "boss/BossRegistry.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace rtype::server {

LevelMode::LevelMode(const level::GameData& data, std::size_t index) : _data(data), _index(index)
{
    if (_index >= _data.levels.size())
        throw std::out_of_range("LevelMode: no level " + std::to_string(_index));
}

void LevelMode::start(GameContext& ctx)
{
    const level::Level& lvl = _data.levels[_index];
    const std::uint32_t seed = lvl.seed ? *lvl.seed : static_cast<std::uint32_t>(ctx.rng());
    _director.emplace(lvl, _data.scaling, seed);
    _terrain.reset();
    if (lvl.terrain)
        _terrain.emplace(*lvl.terrain);
    _boss.reset();
    _bossWait = 0.F;
    _state = ModeState::Running;
    std::cout << "level " << _index + 1 << ": " << lvl.name << '\n';
}

void LevelMode::update(GameContext& ctx, float dt)
{
    if (_state != ModeState::Running || !_director)
        return;
    if (_boss) {
        _boss->update(ctx, dt);
        if (_boss->defeated()) {
            _state = ModeState::Won;
            std::cout << "level " << _index + 1 << " cleared: boss defeated\n";
        }
        return;
    }
    _director->update(ctx, dt);
    if (_terrain)
        _terrain->update(ctx, dt);
    // Waves done when the last wave is gone and the terrain has scrolled past.
    if (!_director->finished() || (_terrain && !_terrain->finished()))
        return;
    const level::Level& lvl = _data.levels[_index];
    if (!lvl.boss) {
        _state = ModeState::Won;
        std::cout << "level " << _index + 1 << " cleared\n";
        return;
    }
    _bossWait += dt;
    if (_bossWait < lvl.boss->delay)
        return;
    // Same scaling as the waves: each extra player adds bossHealth (0.5 = +50%).
    const level::BossDef& def = *lvl.boss->boss;
    const std::size_t players = ctx.players.all().size();
    const float extra = static_cast<float>(players > 1 ? players - 1 : 0);
    const int health = static_cast<int>(
        std::ceil(static_cast<float>(def.health) * (1.F + _data.scaling.bossHealth * extra)));
    _boss = bosses().create(def.id);
    _boss->spawn(ctx, def, health);
    std::cout << "level " << _index + 1 << ": boss " << def.name << " (" << health << " HP)\n";
}

void LevelMode::resolve(GameContext& /*ctx*/, float /*dt*/)
{
    // No respawn
}

std::optional<engine::Entity> LevelMode::spawnShip(GameContext& ctx, network::PeerId peer)
{
    return spawnShipInLane(ctx, peer);
}

ModeState LevelMode::state() const
{
    return _state;
}

std::unique_ptr<IGameMode> LevelMode::next(ModeFactory& modes)
{
    if (_state == ModeState::Lost)
        return modes.lobby(protocol::LobbyPhase::GameOver, selection());

    const std::size_t nextLevel = _index + 1;
    modes.unlock(nextLevel + 1);
    if (nextLevel < _data.levels.size())
        return modes.lobby(protocol::LobbyPhase::LevelCleared,
                           Selection{protocol::GameKind::Level, nextLevel});
    return modes.lobby(protocol::LobbyPhase::CampaignCleared, selection());
}

Selection LevelMode::selection() const
{
    return Selection{protocol::GameKind::Level, _index};
}

std::optional<BossStatus> LevelMode::bossStatus() const
{
    if (!_boss || _state != ModeState::Running)
        return std::nullopt;
    return _boss->status();
}

std::size_t LevelMode::index() const
{
    return _index;
}

} // namespace rtype::server
