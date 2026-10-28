#pragma once

#include "IGameMode.hpp"
#include "LevelData.hpp"

#include <cstddef>
#include <memory>

namespace rtype::server {

class ModeFactory
{
public:
    explicit ModeFactory(const level::GameData& data);

    std::unique_ptr<IGameMode> lobby(protocol::LobbyPhase phase, Selection selection);
    std::unique_ptr<IGameMode> game(Selection selection);

    bool canPlay(Selection selection) const;
    void unlock(std::size_t count);
    std::size_t unlocked() const;

    const level::GameData& data() const;

private:
    const level::GameData& _data;
    std::size_t _unlocked = 1;
};

} // namespace rtype::server
