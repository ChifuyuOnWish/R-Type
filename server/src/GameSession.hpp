#pragma once

#include "EntityFactory.hpp"
#include "EventDispatcher.hpp"
#include "Events.hpp"
#include "GameContext.hpp"
#include "IGameMode.hpp"
#include "Messages.hpp"
#include "NetworkServer.hpp"
#include "Players.hpp"
#include "Registry.hpp"
#include "Replicator.hpp"
#include "Spawner.hpp"
#include "Update.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <random>
#include <vector>

namespace rtype::server {

class GameSession
{
public:
    GameSession(network::NetworkServer& net, std::unique_ptr<IGameMode> mode);
    void run(const std::atomic<bool>& running);
    void setMode(std::unique_ptr<IGameMode> mode);

private:
    void step();
    void handleNetworkEvents();
    void onConnected(const network::Connected& ev);
    void onDisconnected(const network::Disconnected& ev);
    void onMessage(const network::MessageReceived& ev);
    void onDamage(const engine::DamageEvent& ev);
    void onDied(const engine::EntityDiedEvent& ev);

    network::NetworkServer& _net;
    engine::Registry _registry;
    engine::EntityFactory _factory{_registry};
    engine::EventDispatcher _dispatcher;
    engine::Update _loop{_registry};
    std::mt19937 _rng{std::random_device{}()};
    Players _players;
    std::vector<protocol::GameEventMessage> _gameEvents;
    Replicator _replicator{_net, _registry};
    Spawner _spawner{_registry, _factory, _gameEvents};
    GameContext _ctx{_registry, _spawner, _players, _gameEvents, _rng};
    std::unique_ptr<IGameMode> _mode;
    IGameMode* _modePtr = nullptr;
    std::uint32_t _serverTick = 0;
};

} // namespace rtype::server
