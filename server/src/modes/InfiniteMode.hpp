#pragma once

#include "IGameMode.hpp"

namespace rtype::server {

class InfiniteMode final : public IGameMode
{
public:
    void start(GameContext& ctx) override;
    void update(GameContext& ctx, float dt) override;
    void resolve(GameContext& ctx, float dt) override;
    engine::Entity spawnShip(GameContext& ctx, network::PeerId peer) override;
    ModeState state() const override;

private:
    float _spawnTimer = 0.F;
};

} // namespace rtype::server
