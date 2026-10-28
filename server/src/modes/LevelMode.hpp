#pragma once

#include "IGameMode.hpp"
#include "LevelData.hpp"
#include "level/WaveDirector.hpp"

#include <cstddef>
#include <optional>

namespace rtype::server {

class LevelMode final : public IGameMode
{
public:
    LevelMode(const level::GameData& data, std::size_t index);

    void start(GameContext& ctx) override;
    void update(GameContext& ctx, float dt) override;
    void resolve(GameContext& ctx, float dt) override;
    std::optional<engine::Entity> spawnShip(GameContext& ctx, network::PeerId peer) override;
    ModeState state() const override;
    std::unique_ptr<IGameMode> next(ModeFactory& modes) override;
    Selection selection() const override;

    std::size_t index() const;

private:
    const level::GameData& _data;
    std::size_t _index;
    std::optional<level::WaveDirector> _director;
    ModeState _state = ModeState::Running;
};

} // namespace rtype::server
