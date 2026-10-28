#include "LobbyMode.hpp"

#include "ModeFactory.hpp"

#include <iostream>

namespace rtype::server {

namespace {
const char* name(protocol::LobbyPhase phase)
{
    switch (phase) {
    case protocol::LobbyPhase::Selecting:
        return "selecting";
    case protocol::LobbyPhase::LevelCleared:
        return "level cleared";
    case protocol::LobbyPhase::CampaignCleared:
        return "campaign cleared";
    case protocol::LobbyPhase::GameOver:
        return "game over";
    case protocol::LobbyPhase::Playing:
        return "playing";
    }
    return "?";
}
} // namespace

LobbyMode::LobbyMode(const ModeFactory& modes, protocol::LobbyPhase phase, Selection selection)
    : _modes(modes), _phase(phase), _selection(selection)
{
}

void LobbyMode::start(GameContext& /*ctx*/)
{
    std::cout << "lobby: " << name(_phase) << '\n';
}

void LobbyMode::update(GameContext& /*ctx*/, float /*dt*/) {}
void LobbyMode::resolve(GameContext& /*ctx*/, float /*dt*/) {}

std::optional<engine::Entity> LobbyMode::spawnShip(GameContext& /*ctx*/, network::PeerId /*peer*/)
{
    return std::nullopt;
}

ModeState LobbyMode::state() const
{
    return _started ? ModeState::Won : ModeState::Running;
}

std::unique_ptr<IGameMode> LobbyMode::next(ModeFactory& modes)
{
    return modes.game(_selection);
}

std::optional<protocol::LobbyPhase> LobbyMode::lobbyPhase() const
{
    return _phase;
}

Selection LobbyMode::selection() const
{
    return _selection;
}

void LobbyMode::onLobbySelect(const protocol::LobbySelectMessage& msg)
{
    const Selection pick{msg.kind, msg.level};
    if (!_modes.canPlay(pick)) {
        std::cout << "lobby: refused a locked level or unavailable mode\n";
        return;
    }
    _selection = pick;
    _phase = protocol::LobbyPhase::Selecting;
}

void LobbyMode::onLobbyStart()
{
    if (_modes.canPlay(_selection))
        _started = true;
}

} // namespace rtype::server
