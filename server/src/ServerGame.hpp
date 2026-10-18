#pragma once

#include "NetworkServer.hpp"
#include "Registry.hpp"
#include "Update.hpp"

#include <atomic>
#include <cstdint>
#include <map>
#include <string>

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
    };

    void handleNetworkEvents();
    void onConnected(const network::Connected& ev);
    void onDisconnected(const network::Disconnected& ev);
    void onMessage(const network::MessageReceived& ev);
    engine::Entity spawnPlayerShip(network::PeerId id);

    template <typename M>
    void broadcast(const M& msg, bool reliable, network::PeerId except = network::NoPeer);
    template <typename M> void sendTo(network::PeerId peer, const M& msg, bool reliable);

    network::NetworkServer& _net;
    engine::Registry _registry;
    engine::Update _loop{_registry};
    std::map<network::PeerId, Player> _players;
};

} // namespace rtype::server