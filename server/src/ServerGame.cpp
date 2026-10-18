#include "ServerGame.hpp"

#include "Components.hpp"
#include "Messages.hpp"
#include "systems/BoundsSystem.hpp"
#include "systems/InputSystem.hpp"
#include "systems/MotionSystem.hpp"

#include <chrono>
#include <iostream>
#include <thread>

namespace rtype::server {

using namespace engine;
using namespace protocol;
using network::PeerId;

namespace {
constexpr float WorldWidth = 1920.F;
constexpr float WorldHeight = 1080.F;
constexpr float PlayerSpeed = 400.F;
constexpr float BoundsMargin = 100.F;

template <typename M> std::vector<std::uint8_t> encode(const M& msg)
{
    ByteWriter w;
    if (!msg.serialize(w))
        std::cerr << "server: refusing to send an invalid message\n";
    return w.data();
}

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
            onConnected(*c);
        else if (const auto* d = std::get_if<network::Disconnected>(&ev))
            onDisconnected(*d);
        else if (const auto* m = std::get_if<network::MessageReceived>(&ev))
            onMessage(*m);
    }
}

template <typename M> void ServerGame::broadcast(const M& msg, bool reliable, PeerId except)
{
    _net.broadcast(static_cast<std::uint8_t>(M::Type), encode(msg), reliable, except);
}

template <typename M> void ServerGame::sendTo(PeerId peer, const M& msg, bool reliable)
{
    _net.send(peer, static_cast<std::uint8_t>(M::Type), encode(msg), reliable);
}

void ServerGame::onConnected(const network::Connected& ev)
{
    std::cout << "player " << ev.peerId << " joined: " << ev.name << '\n';
    for (const auto& [id, player] : _players)
        sendTo(ev.peerId, PlayerJoinedMessage{static_cast<std::uint8_t>(id), player.name}, true);
    broadcast(PlayerJoinedMessage{static_cast<std::uint8_t>(ev.peerId), ev.name}, true, ev.peerId);
    _players[ev.peerId] = Player{ev.name, spawnPlayerShip(ev.peerId), 0};
}

engine::Entity ServerGame::spawnPlayerShip(PeerId id)
{
    const Entity ship = _registry.createEntity();
    _registry.addComponent(ship, Position{150.F, 200.F * static_cast<float>(id)});
    _registry.addComponent(ship, Velocity{});
    _registry.addComponent(ship, Hitbox{64.F, 32.F});
    _registry.addComponent(ship, Health{3, 3});
    _registry.addComponent(ship, Controllable{});
    _registry.addComponent(ship, PlayerOwner{id});
    return ship;
}

void ServerGame::onMessage(const network::MessageReceived& ev)
{
    if (ev.type != static_cast<std::uint8_t>(MessageType::Input))
        return;
    auto it = _players.find(ev.peerId);
    if (it == _players.end())
        return;
    ByteReader reader(ev.payload.data(), ev.payload.size());
    const auto input = InputMessage::deserialize(reader);
    if (!input)
        return;
    Player& player = it->second;
    if (input->clientTick <= player.lastInputTick)
        return;
    player.lastInputTick = input->clientTick;
    if (!_registry.isAlive(player.ship) || !_registry.hasComponent<Controllable>(player.ship))
        return;
    auto& c = _registry.getComponent<Controllable>(player.ship);
    c.up = (input->buttons & ButtonUp) != 0;
    c.down = (input->buttons & ButtonDown) != 0;
    c.left = (input->buttons & ButtonLeft) != 0;
    c.right = (input->buttons & ButtonRight) != 0;
    c.shoot = (input->buttons & ButtonShoot) != 0;
}

void ServerGame::onDisconnected(const network::Disconnected& ev)
{
    auto it = _players.find(ev.peerId);
    if (it == _players.end())
        return;
    std::cout << "player " << ev.peerId << " left (reason " << static_cast<int>(ev.reason) << ")\n";
    _registry.destroyEntity(it->second.ship);
    _players.erase(it);

    PlayerLeftReason reason = PlayerLeftReason::Quit;
    if (ev.reason == network::DisconnectReason::Timeout)
        reason = PlayerLeftReason::TimedOut;
    else if (ev.reason == network::DisconnectReason::Kicked)
        reason = PlayerLeftReason::Kicked;
    broadcast(PlayerLeftMessage{static_cast<std::uint8_t>(ev.peerId), reason}, true);
}

} // namespace rtype::server
