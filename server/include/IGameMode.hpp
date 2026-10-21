#pragma once

#include "GameContext.hpp"
#include "NetworkEvent.hpp"
#include "ecs/Entity.hpp"

namespace rtype::server {

enum class ModeState
{
    Running,
    Won,
    Lost
};

class IGameMode
{
public:
    virtual ~IGameMode() = default;

    virtual void start(GameContext& ctx) = 0;
    virtual void update(GameContext& ctx, float dt) = 0;
    virtual void resolve(GameContext& ctx, float dt) = 0;
    virtual engine::Entity spawnShip(GameContext& ctx, network::PeerId peer) = 0;
    virtual void onPlayerLeft(GameContext& /*ctx*/, network::PeerId /*peer*/) {}

    virtual ModeState state() const = 0;
};

} // namespace rtype::server
