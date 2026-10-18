#pragma once

#include "GameLoop.hpp"
#include "NetworkServer.hpp"
#include "Registry.hpp"

#include <atomic>

namespace rtype::server {

class ServerGame
{
public:
    explicit ServerGame(network::NetworkServer& net);
    void run(const std::atomic<bool>& running);

private:
    void handleNetworkEvents();

    network::NetworkServer& _net;
    engine::Registry _registry;
    engine::GameLoop _loop{_registry};
};

} // namespace rtype::server