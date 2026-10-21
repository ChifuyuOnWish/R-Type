#include "GameSession.hpp"

#include "ModeSystems.hpp"
#include "World.hpp"
#include "components/Components.hpp"
#include "systems/BoundsSystem.hpp"
#include "systems/CollisionSystem.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/DeathSystem.hpp"
#include "systems/EnemyBehaviorSystem.hpp"
#include "systems/InputSystem.hpp"
#include "systems/LifetimeSystem.hpp"
#include "systems/MotionSystem.hpp"
#include "systems/WeaponSystem.hpp"

#include <chrono>
#include <exception>
#include <iostream>
#include <thread>
#include <utility>

namespace rtype::server {

using namespace engine;
using namespace protocol;

namespace {
constexpr float PlayerSpeed = 400.F;
constexpr float BoundsMargin = 100.F;
} // namespace

GameSession::GameSession(network::NetworkServer& net, std::unique_ptr<IGameMode> mode) : _net(net)
{
    _dispatcher.subscribe<DamageEvent>([this](const DamageEvent& ev) { onDamage(ev); });
    _dispatcher.subscribe<EntityDiedEvent>([this](const EntityDiedEvent& ev) { onDied(ev); });

    _loop.addSystem(std::make_unique<InputSystem>(PlayerSpeed));
    _loop.addSystem(std::make_unique<ModeUpdateSystem>(_modePtr, _ctx));
    _loop.addSystem(std::make_unique<EnemyBehaviorSystem>());
    _loop.addSystem(std::make_unique<WeaponSystem>(
        [this](Registry& r, Entity shooter, Position from, unsigned type) {
            _spawner.spawnProjectile(r, shooter, from, type);
        }));
    _loop.addSystem(std::make_unique<MotionSystem>());
    _loop.addSystem(std::make_unique<CollisionSystem>());
    _loop.addSystem(std::make_unique<DamageSystem>(&_dispatcher));
    _loop.addSystem(std::make_unique<ModeResolveSystem>(_modePtr, _ctx));
    _loop.addSystem(std::make_unique<DeathSystem>());
    _loop.addSystem(std::make_unique<LifetimeSystem>());
    _loop.addSystem(std::make_unique<BoundsSystem>(BoundsArea{0.F, 0.F, WorldWidth, WorldHeight},
                                                   BoundsMargin));

    setMode(std::move(mode));
}

void GameSession::setMode(std::unique_ptr<IGameMode> mode)
{
    _mode = std::move(mode);
    _modePtr = _mode.get();
    if (_mode)
        _mode->start(_ctx);
}

void GameSession::run(const std::atomic<bool>& running)
{
    while (running.load()) {
        try {
            step();
        } catch (const std::exception& e) {
            std::cerr << "server: game tick failed: " << e.what() << '\n';
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void GameSession::step()
{
    handleNetworkEvents();
    const unsigned steps = _loop.tick();
    if (steps == 0)
        return;
    _serverTick += steps;
    _dispatcher.flush();
    _replicator.sync();
    _replicator.sendEvents(_gameEvents);
    _replicator.afterSteps(steps, _serverTick);
}

void GameSession::handleNetworkEvents()
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

void GameSession::onConnected(const network::Connected& ev)
{
    std::cout << "player " << ev.peerId << " joined: " << ev.name << '\n';
    for (const auto& [id, player] : _players.all())
        _replicator.sendTo(ev.peerId,
                           PlayerJoinedMessage{static_cast<std::uint8_t>(id), player.name}, true);
    _replicator.broadcast(PlayerJoinedMessage{static_cast<std::uint8_t>(ev.peerId), ev.name}, true,
                          ev.peerId);
    _replicator.sendWorldTo(ev.peerId);
    _players.add(ev.peerId, ev.name, _mode->spawnShip(_ctx, ev.peerId));
}

void GameSession::onMessage(const network::MessageReceived& ev)
{
    if (ev.type != static_cast<std::uint8_t>(MessageType::Input))
        return;
    ByteReader reader(ev.payload.data(), ev.payload.size());
    const auto input = InputMessage::deserialize(reader);
    if (!input)
        return;
    _players.applyInput(ev.peerId, *input, _registry);
}

void GameSession::onDisconnected(const network::Disconnected& ev)
{
    const auto player = _players.remove(ev.peerId);
    if (!player)
        return;
    std::cout << "player " << ev.peerId << " left (reason " << static_cast<int>(ev.reason) << ")\n";
    _mode->onPlayerLeft(_ctx, ev.peerId);
    _replicator.forget(player->ship);
    _registry.destroyEntity(player->ship);

    PlayerLeftReason reason = PlayerLeftReason::Quit;
    if (ev.reason == network::DisconnectReason::Timeout)
        reason = PlayerLeftReason::TimedOut;
    else if (ev.reason == network::DisconnectReason::Kicked)
        reason = PlayerLeftReason::Kicked;
    _replicator.broadcast(PlayerLeftMessage{static_cast<std::uint8_t>(ev.peerId), reason}, true);
}

void GameSession::onDamage(const DamageEvent& ev)
{
    if (!_players.isShip(ev.victim) || !_registry.isAlive(ev.victim))
        return;
    if (const std::uint32_t target = _replicator.wireId(ev.victim))
        _gameEvents.push_back(
            GameEventMessage{GameEventType::Hit, _replicator.wireId(ev.source), target});
}

void GameSession::onDied(const EntityDiedEvent& ev)
{
    const std::uint32_t target = _replicator.wireId(ev.entity);
    if (target == 0)
        return;
    const auto type = _players.isShip(ev.entity) ? GameEventType::PlayerDeath : GameEventType::Kill;
    _gameEvents.push_back(GameEventMessage{type, _replicator.wireId(ev.killer), target});
}

} // namespace rtype::server
