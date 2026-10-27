#pragma once

#include "IGameMode.hpp"

namespace rtype::server {

class InfiniteMode final : public IGameMode
{
public:
    void start(GameContext& ctx) override;
    void update(GameContext& ctx, float dt) override;
    void resolve(GameContext& ctx, float dt) override;
    std::optional<engine::Entity> spawnShip(GameContext& ctx, network::PeerId peer) override;
    ModeState state() const override;
    std::unique_ptr<IGameMode> next(ModeFactory& modes) override;
    Selection selection() const override;

private:
    float _spawnTimer = 0.F;
};

} // namespace rtype::server
