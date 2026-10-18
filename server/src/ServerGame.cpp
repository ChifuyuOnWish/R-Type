#include "ServerGame.hpp"

#include "Components.hpp"
#include "EntityType.hpp"
#include "Messages.hpp"
#include "ServerComponents.hpp"
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

constexpr unsigned StepsPerSnapshot = 3;

bool outsideWorld(float x, float y)
{
    return x < 0.F || x > WorldWidth || y < 0.F || y > WorldHeight;
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
        const unsigned steps = _loop.tick();
        if (steps > 0) {
            _serverTick += steps;
            syncEntities();
            _stepsSinceSnapshot += steps;
            if (_stepsSinceSnapshot >= StepsPerSnapshot) {
                sendSnapshot();
                _stepsSinceSnapshot = 0;
            }
        }
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
    for (const auto& [netId, s] : _synced) {
        if (!_registry.isAlive(s.entity) || !_registry.hasComponent<Position>(s.entity))
            continue;
        const auto& p = _registry.getComponent<Position>(s.entity);
        sendTo(ev.peerId,
               EntitySpawnMessage{netId, s.type, Fixed::fromFloat(p.x), Fixed::fromFloat(p.y)},
               true);
    }
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
    _registry.addComponent(ship, NetType{static_cast<std::uint16_t>(playerShipType(id))});
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
    const Entity ship = it->second.ship;
    if (const std::uint32_t netId = netIdOf(_registry, ship)) {
        broadcast(EntityDestroyMessage{netId, DestroyCause::Other}, true);
        _synced.erase(netId);
    }
    _registry.destroyEntity(ship);
    _players.erase(it);

    PlayerLeftReason reason = PlayerLeftReason::Quit;
    if (ev.reason == network::DisconnectReason::Timeout)
        reason = PlayerLeftReason::TimedOut;
    else if (ev.reason == network::DisconnectReason::Kicked)
        reason = PlayerLeftReason::Kicked;
    broadcast(PlayerLeftMessage{static_cast<std::uint8_t>(ev.peerId), reason}, true);
}

void ServerGame::syncEntities()
{
    _registry.each<NetType, Position>([&](Entity e, NetType& t, Position& p) {
        if (_registry.hasComponent<NetId>(e))
            return;
        const std::uint32_t netId = _nextNetId++;
        const Synced synced{e, t.type, p.x, p.y};
        _registry.addComponent(e, NetId{netId});
        _synced[netId] = synced;
        broadcast(EntitySpawnMessage{netId, synced.type, Fixed::fromFloat(synced.x),
                                     Fixed::fromFloat(synced.y)},
                  true);
    });
    for (auto it = _synced.begin(); it != _synced.end();) {
        Synced& s = it->second;
        if (_registry.isAlive(s.entity)) {
            if (_registry.hasComponent<Position>(s.entity)) {
                const auto& p = _registry.getComponent<Position>(s.entity);
                s.x = p.x;
                s.y = p.y;
            }
            ++it;
            continue;
        }
        const auto cause = outsideWorld(s.x, s.y) ? DestroyCause::LeftScreen : DestroyCause::Killed;
        broadcast(EntityDestroyMessage{it->first, cause}, true);
        it = _synced.erase(it);
    }
}

void ServerGame::sendSnapshot()
{
    constexpr float PerTick = static_cast<float>(Update::Step);
    SnapshotMessage snap{_serverTick, {}};
    for (const auto& [netId, s] : _synced) {
        SnapshotEntry e{netId, Fixed::fromFloat(s.x), Fixed::fromFloat(s.y), {}, {}};
        if (_registry.hasComponent<Velocity>(s.entity)) {
            const auto& v = _registry.getComponent<Velocity>(s.entity);
            e.vx = Fixed16::fromFloat(v.dx * PerTick);
            e.vy = Fixed16::fromFloat(v.dy * PerTick);
        }
        snap.entries.push_back(e);
        if (snap.entries.size() == MaxSnapshotEntries) {
            broadcast(snap, false);
            snap.entries.clear();
        }
    }
    if (!snap.entries.empty() || _synced.empty())
        broadcast(snap, false);
}

} // namespace rtype::server
