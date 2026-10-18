#include "ServerGame.hpp"

#include "systems/BoundsSystem.hpp"
#include "systems/InputSystem.hpp"
#include "systems/MotionSystem.hpp"

#include <chrono>
#include <iostream>
#include <thread>

namespace rtype::server {

using namespace engine;

namespace {
constexpr float WorldWidth = 1920.F;
constexpr float WorldHeight = 1080.F;
constexpr float PlayerSpeed = 400.F;
constexpr float BoundsMargin = 100.F;
} // namespace

ServerGame::ServerGame(network::NetworkServer& net) : _net(net)
{
    _loop.addSystem(std::make_unique<InputSystem>(PlayerSpeed));
    _loop.addSystem(std::make_unique<MotionSystem>());
    _loop.addSystem(std::make_unique<BoundsSystem>(BoundsArea{0.F, 0.F, WorldWidth, WorldHeight},
                                                   BoundsMargin));
}

void ServerGame::run(const std::atomic<bool>& running)
{
    while (running.load()) {
        handleNetworkEvents();
        _loop.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void ServerGame::handleNetworkEvents()
{
    for (auto& ev : _net.poll()) {
        if (const auto* c = std::get_if<network::Connected>(&ev))
            std::cout << "player " << c->peerId << " joined: " << c->name << '\n';
        else if (const auto* d = std::get_if<network::Disconnected>(&ev))
            std::cout << "player " << d->peerId << " left (reason " << static_cast<int>(d->reason)
                      << ")\n";
    }
}

} // namespace rtype::server