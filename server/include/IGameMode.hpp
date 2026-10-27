#pragma once

#include "GameContext.hpp"
#include "LobbyMessages.hpp"
#include "NetworkEvent.hpp"
#include "ecs/Entity.hpp"

#include <cstddef>
#include <memory>
#include <optional>

namespace rtype::server {

class ModeFactory;

enum class ModeState
{
    Running,
    Won,
    Lost
};

struct Selection
{
    protocol::GameKind kind = protocol::GameKind::Level;
    std::size_t level = 0;
};

class IGameMode
{
public:
    virtual ~IGameMode() = default;

    virtual void start(GameContext& ctx) = 0;
    virtual void update(GameContext& ctx, float dt) = 0;
    virtual void resolve(GameContext& ctx, float dt) = 0;
    virtual std::optional<engine::Entity> spawnShip(GameContext& ctx, network::PeerId peer) = 0;
    virtual void onPlayerLeft(GameContext& /*ctx*/, network::PeerId /*peer*/) {}

    virtual ModeState state() const = 0;
    virtual std::unique_ptr<IGameMode> next(ModeFactory& modes) = 0;

    virtual std::optional<protocol::LobbyPhase> lobbyPhase() const { return std::nullopt; }
    virtual Selection selection() const { return {}; }
    virtual void onLobbySelect(const protocol::LobbySelectMessage& /*msg*/) {}
    virtual void onLobbyStart() {}
};

} // namespace rtype::server
