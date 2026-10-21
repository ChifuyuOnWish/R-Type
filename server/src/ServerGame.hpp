#pragma once

#include "Events.hpp"
#include "Messages.hpp"
#include "NetworkServer.hpp"
#include "core/EventDispatcher.hpp"
#include "core/Update.hpp"
#include "ecs/EntityFactory.hpp"
#include "ecs/Registry.hpp"

#include <atomic>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace rtype::server {

class ServerGame
{
public:
    explicit ServerGame(network::NetworkServer& net);
    void run(const std::atomic<bool>& running);

private:
    struct Player
    {
        std::string name;
        engine::Entity ship;
        std::uint32_t lastInputTick = 0;
        bool hasInput = false;
    };
    struct Synced
    {
        engine::Entity entity;
        std::uint16_t type;
        float x;
        float y;
    };

    void step();
    void handleNetworkEvents();
    void onConnected(const network::Connected& ev);
    void onDisconnected(const network::Disconnected& ev);
    void onMessage(const network::MessageReceived& ev);
    engine::Entity spawnPlayerShip(network::PeerId id);
    void spawnProjectile(engine::Registry& registry, engine::Entity shooter, engine::Position from,
                         unsigned type);

    void onDamage(const engine::DamageEvent& ev);
    void onDied(const engine::EntityDiedEvent& ev);
    std::uint32_t wireId(engine::Entity entity) const;
    bool isShip(engine::Entity entity) const;

    template <typename M>
    void broadcast(const M& msg, bool reliable, network::PeerId except = network::NoPeer);
    template <typename M> void sendTo(network::PeerId peer, const M& msg, bool reliable);

    void syncEntities();
    void sendGameEvents();
    void sendSnapshot();

    network::NetworkServer& _net;
    engine::Registry _registry;
    engine::EntityFactory _factory{_registry};
    engine::EventDispatcher _dispatcher;
    engine::Update _loop{_registry};
    std::map<network::PeerId, Player> _players;
    std::map<std::uint32_t, Synced> _synced;
    std::vector<protocol::GameEventMessage> _gameEvents;
    std::uint32_t _nextNetId = 1;
    std::uint32_t _serverTick = 0;
    unsigned _stepsSinceSnapshot = 0;
};

} // namespace rtype::server
