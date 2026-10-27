#include "ModeFactory.hpp"

// The modes are included here, not in the header: LobbyMode.hpp and ModeFactory.hpp would
// otherwise include each other.
#include "InfiniteMode.hpp"
#include "LevelMode.hpp"
#include "LobbyMode.hpp"

#include <algorithm>
#include <iostream>

namespace rtype::server {

ModeFactory::ModeFactory(const level::GameData& data) : _data(data) {}

std::unique_ptr<IGameMode> ModeFactory::lobby(protocol::LobbyPhase phase, Selection selection)
{
    return std::make_unique<LobbyMode>(*this, phase, selection);
}

std::unique_ptr<IGameMode> ModeFactory::game(Selection selection)
{
    switch (selection.kind) {
    case protocol::GameKind::Level:
        return std::make_unique<LevelMode>(_data, selection.level);
    case protocol::GameKind::Infinite:
        return std::make_unique<InfiniteMode>();
    case protocol::GameKind::Pvp:
        break;
    }
    std::cerr << "server: game mode not available yet, back to the lobby\n";
    return lobby(protocol::LobbyPhase::Selecting, selection);
}

bool ModeFactory::canPlay(Selection selection) const
{
    switch (selection.kind) {
    case protocol::GameKind::Level:
        return selection.level < _unlocked;
    case protocol::GameKind::Infinite:
        return true;
    case protocol::GameKind::Pvp:
        return false;
    }
    return false;
}

void ModeFactory::unlock(std::size_t count)
{
    const std::size_t total = std::max<std::size_t>(_data.levels.size(), 1);
    _unlocked = std::clamp(std::max(_unlocked, count), std::size_t{1}, total);
}

std::size_t ModeFactory::unlocked() const
{
    return _unlocked;
}

const level::GameData& ModeFactory::data() const
{
    return _data;
}

} // namespace rtype::server
