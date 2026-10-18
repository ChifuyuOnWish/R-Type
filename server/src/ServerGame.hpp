#pragma once

#include "NetworkServer.hpp"
#include "Registry.hpp"
#include "Update.hpp"

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
    engine::Update _loop{_registry};
};

} // namespace rtype::server