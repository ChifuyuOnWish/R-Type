#include "GameSession.hpp"

#include "ModeSystems.hpp"
#include "ServerComponents.hpp"
#include "World.hpp"
#include "components/Components.hpp"
#include "monsters/MonsterSystem.hpp"
#include "systems/BoundsSystem.hpp"
#include "systems/CollisionSystem.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/DeathSystem.hpp"
#include "systems/InputSystem.hpp"
#include "systems/LifetimeSystem.hpp"
#include "systems/MotionSystem.hpp"
#include "systems/WeaponSystem.hpp"
#include "terrain/TerrainSystem.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
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
constexpr std::uint32_t KillPoints = 100;
constexpr float ComboWindow = 3.F;
constexpr std::uint8_t MaxCombo = 8;
constexpr std::uint8_t PlayerMaxHealth = 3;
// Health alone changes with every hit: send it at most every 6 steps (100 ms at 60 Hz).
// Phase changes go out at once.
constexpr std::uint32_t BossHealthEvery = 6;
} // namespace

GameSession::GameSession(network::NetworkServer& net, const level::GameData& data)
    : _net(net), _modes(data)
{
    _dispatcher.subscribe<DamageEvent>([this](const DamageEvent& ev) { onDamage(ev); });
    _dispatcher.subscribe<EntityDiedEvent>([this](const EntityDiedEvent& ev) { onDied(ev); });

    _loop.addSystem(std::make_unique<InputSystem>(PlayerSpeed));
    _loop.addSystem(std::make_unique<ModeUpdateSystem>(_modePtr, _ctx));

    _loop.addSystem(std::make_unique<MonsterSystem>(_rng));
    _loop.addSystem(std::make_unique<WeaponSystem>(
        [this](Registry& r, Entity shooter, Position from, unsigned type) {
            _spawner.spawnProjectile(r, shooter, from, type);
        }));
    _loop.addSystem(std::make_unique<MotionSystem>());

    _loop.addSystem(std::make_unique<TerrainSystem>());
    _loop.addSystem(std::make_unique<CollisionSystem>());
    _loop.addSystem(std::make_unique<DamageSystem>(&_dispatcher));
    _loop.addSystem(std::make_unique<ModeResolveSystem>(_modePtr, _ctx));
    _loop.addSystem(std::make_unique<DeathSystem>());
    _loop.addSystem(std::make_unique<LifetimeSystem>());

    _loop.addSystem(std::make_unique<BoundsSystem>(BoundsArea{0.F, PlayTop, WorldWidth, PlayBottom},
                                                   BoundsMargin));

    switchMode(_modes.lobby(LobbyPhase::Selecting, Selection{}));
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
    scoreBoss();
    _players.refreshDeaths(_registry);
    if (_comboTimer > 0.F) {
        _comboTimer -= static_cast<float>(steps) * static_cast<float>(Update::Step);
        if (_comboTimer <= 0.F)
            _combo = 1;
    }
    _replicator.sync();
    _replicator.sendEvents(_gameEvents);
    _replicator.afterSteps(steps, _serverTick);

    if (_mode->state() != ModeState::Running) {
        switchMode(_mode->next(_modes));
    } else if (_players.all().empty() && !_mode->lobbyPhase()) {
        switchMode(_modes.lobby(LobbyPhase::Selecting, _mode->selection()));
    } else if (!_mode->lobbyPhase() && _players.allDead()) {
        std::cout << "all players are dead: game over\n";
        switchMode(_modes.lobby(LobbyPhase::GameOver, _mode->selection()));
    }
    _replicator.sendScores(scoreState());
    sendLobbyState();
    sendBossState(); // after sync(): clients already know the boss entity it points to
}

void GameSession::switchMode(std::unique_ptr<IGameMode> mode)
{
    clearWorld();
    _mode = std::move(mode);
    _modePtr = _mode.get();
    _mode->start(_ctx);
    if (_mode->lobbyPhase()) {
        _combo = 1;
        _comboTimer = 0.F;
    } else {
        _players.resetScores();
    }
    for (const auto& [peer, player] : _players.all())
        _players.setShip(peer, _mode->spawnShip(_ctx, peer));
}

void GameSession::clearWorld()
{
    std::vector<Entity> doomed;
    _registry.each<NetType>([&doomed](Entity e, NetType&) { doomed.push_back(e); });
    for (const Entity e : doomed)
        _registry.destroyEntity(e);
    for (const auto& [peer, player] : _players.all())
        _players.setShip(peer, std::nullopt);
}

LobbyStateMessage GameSession::lobbyState() const
{
    LobbyStateMessage state;
    state.phase = _mode->lobbyPhase().value_or(LobbyPhase::Playing);
    const auto leader = _players.leader();
    state.leaderId = leader ? static_cast<std::uint8_t>(*leader) : 0;
    const Selection selection = _mode->selection();
    state.kind = selection.kind;
    state.level = static_cast<std::uint8_t>(selection.level);
    state.unlocked = static_cast<std::uint8_t>(_modes.unlocked());
    for (const auto& level : _modes.data().levels)
        state.levelNames.push_back(level.name);
    return state;
}

void GameSession::sendLobbyState()
{
    LobbyStateMessage state = lobbyState();
    if (_lastLobbyState && *_lastLobbyState == state)
        return;
    _replicator.broadcast(state, true);
    _lastLobbyState = std::move(state);
}

BossStateMessage GameSession::bossState() const
{
    BossStateMessage msg;
    const auto boss = _mode->bossStatus();
    if (!boss)
        return msg; // entityId 0: no boss, hide the bar
    msg.entityId = _replicator.wireId(boss->body);
    if (msg.entityId == 0)
        return msg;
    msg.phase = boss->phase;
    msg.phaseCount = boss->phaseCount;
    msg.maxHealth = static_cast<std::uint16_t>(std::clamp(boss->maxHealth, 1, 65535));
    msg.health = static_cast<std::uint16_t>(std::clamp(boss->health, 0, int{msg.maxHealth}));
    msg.flags = static_cast<std::uint8_t>((boss->exposed ? BossStateMessage::Exposed : 0) |
                                          (boss->enraged ? BossStateMessage::Enraged : 0) |
                                          (boss->dying ? BossStateMessage::Dying : 0));
    for (const float mark : boss->marks)
        if (msg.marks.size() < MaxBossPhases)
            msg.marks.push_back(static_cast<std::uint8_t>(std::lround(mark * 100.F)));
    msg.name = boss->name;
    return msg;
}

void GameSession::sendBossState()
{
    BossStateMessage state = bossState();
    if (!_lastBossState && state.entityId == 0)
        return; // no boss before, none now
    if (_lastBossState && *_lastBossState == state)
        return;
    // Only the health moved: wait a little so 4 players firing don't flood the reliable channel.
    BossStateMessage healthOnly = state;
    if (_lastBossState) {
        healthOnly.health = _lastBossState->health;
        if (healthOnly == *_lastBossState && _serverTick - _bossSentAt < BossHealthEvery)
            return;
    }
    _replicator.broadcast(state, true);
    _bossSentAt = _serverTick;
    if (state.entityId == 0)
        _lastBossState.reset();
    else
        _lastBossState = std::move(state);
}

// The boss never dies through DamageSystem (its pieces are armoured), so onDied can't score
// it. The ship that hit it last when its health reaches 0 gets its "score" from enemies.json,
// times the combo, like any other kill.
void GameSession::scoreBoss()
{
    const auto boss = _mode->bossStatus();
    if (!boss) {
        _bossScored = false;
        _bossHitter = Entity{};
        return;
    }
    if (!boss->dying || _bossScored)
        return;
    _bossScored = true;
    const auto points = static_cast<std::uint32_t>(std::max(boss->score, 0));
    if (_players.addScore(_bossHitter, points * _combo)) {
        if (_combo < MaxCombo)
            ++_combo;
        _comboTimer = ComboWindow;
    }
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
    _replicator.sendTo(ev.peerId, lobbyState(), true);
    _replicator.sendTo(ev.peerId, scoreState(), true);
    if (_lastBossState) // joined mid-fight
        _replicator.sendTo(ev.peerId, *_lastBossState, true);
}

void GameSession::onMessage(const network::MessageReceived& ev)
{
    ByteReader reader(ev.payload.data(), ev.payload.size());
    switch (static_cast<MessageType>(ev.type)) {
    case MessageType::Input:
        if (const auto input = InputMessage::deserialize(reader))
            _players.applyInput(ev.peerId, *input, _registry);
        break;
    case MessageType::LobbySelect:
        if (_players.leader() != ev.peerId)
            break;
        if (const auto select = LobbySelectMessage::deserialize(reader))
            _mode->onLobbySelect(*select);
        break;
    case MessageType::LobbyStart:
        if (_players.leader() != ev.peerId)
            break;
        if (LobbyStartMessage::deserialize(reader))
            _mode->onLobbyStart();
        break;
    default:
        break;
    }
}

void GameSession::onDisconnected(const network::Disconnected& ev)
{
    const auto player = _players.remove(ev.peerId);
    if (!player)
        return;
    std::cout << "player " << ev.peerId << " left (reason " << static_cast<int>(ev.reason) << ")\n";
    _mode->onPlayerLeft(_ctx, ev.peerId);
    if (player->ship && _registry.isAlive(*player->ship)) {
        _replicator.forget(*player->ship);
        _registry.destroyEntity(*player->ship);
    }

    PlayerLeftReason reason = PlayerLeftReason::Quit;
    if (ev.reason == network::DisconnectReason::Timeout)
        reason = PlayerLeftReason::TimedOut;
    else if (ev.reason == network::DisconnectReason::Kicked)
        reason = PlayerLeftReason::Kicked;
    _replicator.broadcast(PlayerLeftMessage{static_cast<std::uint8_t>(ev.peerId), reason}, true);
}

void GameSession::onDamage(const DamageEvent& ev)
{
    // Boss pieces get hit like anything else: remember who hit it last, for the kill points.
    if (const auto boss = _mode->bossStatus(); boss && ev.victim == boss->body)
        _bossHitter = ev.source;
    if (!_players.isShip(ev.victim) || !_registry.isAlive(ev.victim))
        return;
    _combo = 1;
    _comboTimer = 0.F;
    if (const std::uint32_t target = _replicator.wireId(ev.victim))
        _gameEvents.push_back(
            GameEventMessage{GameEventType::Hit, _replicator.wireId(ev.source), target});
}

void GameSession::onDied(const EntityDiedEvent& ev)
{
    const bool ship = _players.isShip(ev.entity);
    if (!ship) {
        Entity shooter = ev.killer;
        if (_registry.isAlive(shooter) && _registry.hasComponent<Projectile>(shooter))
            shooter = _registry.getComponent<Projectile>(shooter).owner;
        if (_players.addScore(shooter, KillPoints * _combo)) {
            if (_combo < MaxCombo)
                ++_combo;
            _comboTimer = ComboWindow;
        }
    } else {
        _combo = 1;
    }
    const std::uint32_t target = _replicator.wireId(ev.entity);
    if (target == 0)
        return;
    const auto type = ship ? GameEventType::PlayerDeath : GameEventType::Kill;
    _gameEvents.push_back(GameEventMessage{type, _replicator.wireId(ev.killer), target});
}

ScoreUpdateMessage GameSession::scoreState() const
{
    ScoreUpdateMessage state;
    state.teamScore = _players.teamScore();
    state.combo = _combo;
    for (const auto& [peer, player] : _players.all()) {
        if (state.players.size() == MaxScoreEntries)
            break;
        std::uint8_t hp = 0;
        if (!player.dead && player.ship && _registry.isAlive(*player.ship) &&
            _registry.hasComponent<Health>(*player.ship))
            hp = static_cast<std::uint8_t>(
                std::clamp(_registry.getComponent<Health>(*player.ship).current, 0,
                           static_cast<int>(PlayerMaxHealth)));
        state.players.push_back(
            {static_cast<std::uint8_t>(peer), player.score, hp, PlayerMaxHealth, player.kills});
    }
    return state;
}

} // namespace rtype::server
