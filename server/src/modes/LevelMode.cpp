#include "LevelMode.hpp"

#include "ModeFactory.hpp"
#include "ShipRules.hpp"

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
    _state = ModeState::Running;
    std::cout << "level " << _index + 1 << ": " << lvl.name << '\n';
    if (lvl.terrain || lvl.boss)
        std::cout << "level " << _index + 1 << ": terrain and boss not played yet\n";
}

void LevelMode::update(GameContext& ctx, float dt)
{
    if (_state != ModeState::Running || !_director)
        return;
    _director->update(ctx, dt);
    if (_director->finished()) {
        _state = ModeState::Won;
        std::cout << "level " << _index + 1 << " cleared\n";
    }
}

void LevelMode::resolve(GameContext& ctx, float /*dt*/)
{
    respawnDeadShips(ctx.registry);
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
    if (_state == ModeState::Lost) // once lives exist: retry is one click
        return modes.lobby(protocol::LobbyPhase::GameOver, selection());

    const std::size_t nextLevel = _index + 1;
    modes.unlock(nextLevel + 1); // count, not index: levels 0 .. nextLevel are open
    if (nextLevel < _data.levels.size())
        return modes.lobby(protocol::LobbyPhase::LevelCleared,
                           Selection{protocol::GameKind::Level, nextLevel});
    return modes.lobby(protocol::LobbyPhase::CampaignCleared, selection());
}

Selection LevelMode::selection() const
{
    return Selection{protocol::GameKind::Level, _index};
}

std::size_t LevelMode::index() const
{
    return _index;
}

} // namespace rtype::server
