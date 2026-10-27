#pragma once

#include "IGameMode.hpp"

namespace rtype::server {

class ModeFactory;

class LobbyMode final : public IGameMode
{
public:
    LobbyMode(const ModeFactory& modes, protocol::LobbyPhase phase, Selection selection);

    void start(GameContext& ctx) override;
    void update(GameContext& ctx, float dt) override;
    void resolve(GameContext& ctx, float dt) override;
    std::optional<engine::Entity> spawnShip(GameContext& ctx, network::PeerId peer) override;
    ModeState state() const override;
    std::unique_ptr<IGameMode> next(ModeFactory& modes) override;

    std::optional<protocol::LobbyPhase> lobbyPhase() const override;
    Selection selection() const override;
    void onLobbySelect(const protocol::LobbySelectMessage& msg) override;
    void onLobbyStart() override;

private:
    const ModeFactory& _modes;
    protocol::LobbyPhase _phase;
    Selection _selection;
    bool _started = false;
};

} // namespace rtype::server
