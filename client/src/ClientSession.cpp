#include "ClientSession.hpp"

#include "systems/RenderSystem.hpp"

#include <algorithm>
#include <stdexcept>

namespace rtype::client {
namespace {
namespace proto = protocol;
namespace net = network;
namespace ui = gui;

template <typename Message> std::vector<std::uint8_t> encode(const Message& message)
{
    proto::ByteWriter writer;
    if (!message.serialize(writer))
        throw std::runtime_error("Could not encode a client message");
    return writer.data();
}
proto::GameKind kind(ui::FrontendMode mode)
{
    if (mode == ui::FrontendMode::Infinite)
        return proto::GameKind::Infinite;
    if (mode == ui::FrontendMode::Duel)
        return proto::GameKind::Pvp;
    return proto::GameKind::Level;
}
ui::FrontendMode mode(proto::GameKind value)
{
    if (value == proto::GameKind::Infinite)
        return ui::FrontendMode::Infinite;
    if (value == proto::GameKind::Pvp)
        return ui::FrontendMode::Duel;
    return ui::FrontendMode::Campaign;
}
std::string failure(net::ConnectFailure reason)
{
    switch (reason) {
    case net::ConnectFailure::Full:
        return "The server is full.";
    case net::ConnectFailure::BadVersion:
        return "Client and server versions do not match.";
    case net::ConnectFailure::Banned:
        return "The server rejected this connection.";
    case net::ConnectFailure::Timeout:
        return "The server did not respond. Check the address and retry.";
    }
    return "Connection failed.";
}
std::string disconnected(net::DisconnectReason reason)
{
    switch (reason) {
    case net::DisconnectReason::Quit:
        return "Disconnected.";
    case net::DisconnectReason::Timeout:
        return "Connection lost: the server stopped responding.";
    case net::DisconnectReason::Kicked:
        return "You were disconnected by the server.";
    case net::DisconnectReason::ServerShutdown:
        return "The server shut down.";
    }
    return "Disconnected.";
}
} // namespace

ClientSession::ClientSession(gui::ConnectionDetails connection)
    : _world(std::make_unique<ClientWorld>())
{
    _frontend.page = ui::FrontendPage::Connection;
    _frontend.connection = std::move(connection);
}
void ClientSession::resetWorld()
{
    _world = std::make_unique<ClientWorld>();
    _names.clear();
    _sounds.clear();
    _frontend.selectionPending = false;
    _frontend.leader = false;
    _frontend.levels.clear();
    _frontend.unlocked = 0;
}
void ClientSession::connect(const ui::ConnectionDetails& connection)
{
    _network.reset();
    resetWorld();
    _clientTick = 1;
    _frontend.connection = connection;
    _frontend.page = ui::FrontendPage::Connecting;
    _frontend.message = connection.host + ":" + std::to_string(connection.port);
    _network = std::make_unique<ClientNetwork>();
    installHandlers();
    if (const auto error = _network->connect(connection.host, connection.port, connection.name)) {
        _frontend.page = ui::FrontendPage::Error;
        _frontend.message = "Could not connect: " + error.message();
    }
}
void ClientSession::installHandlers()
{
    _network->onConnected([this](net::PeerId id, const std::string& name) {
        _names[id] = name;
        _frontend.page = ui::FrontendPage::Waiting;
        _frontend.message = "Connected. Waiting for lobby information.";
        _waitingSince = std::chrono::steady_clock::now();
    });
    _network->onConnectionFailed([this](net::ConnectFailure reason) {
        resetWorld();
        _frontend.page = ui::FrontendPage::Error;
        _frontend.message = failure(reason);
    });
    _network->onDisconnected([this](net::PeerId, net::DisconnectReason reason) {
        resetWorld();
        _frontend.page = ui::FrontendPage::Connection;
        _frontend.message = disconnected(reason);
    });
    _network->registerHandler(
        proto::MessageType::LobbyState, [this](std::span<const std::uint8_t> payload) {
            proto::ByteReader reader(payload.data(), payload.size());
            if (const auto state = proto::LobbyStateMessage::deserialize(reader))
                lobby(*state);
        });
    _network->registerHandler(
        proto::MessageType::PlayerJoined, [this](std::span<const std::uint8_t> payload) {
            proto::ByteReader reader(payload.data(), payload.size());
            if (const auto message = proto::PlayerJoinedMessage::deserialize(reader))
                _names[message->clientId] = message->name;
        });
    _network->registerHandler(
        proto::MessageType::PlayerLeft, [this](std::span<const std::uint8_t> payload) {
            proto::ByteReader reader(payload.data(), payload.size());
            if (const auto message = proto::PlayerLeftMessage::deserialize(reader))
                _names.erase(message->clientId);
        });
    _network->registerHandler(
        proto::MessageType::EntitySpawn, [this](std::span<const std::uint8_t> payload) {
            proto::ByteReader reader(payload.data(), payload.size());
            if (const auto message = proto::EntitySpawnMessage::deserialize(reader))
                _world->spawn(*message);
        });
    _network->registerHandler(
        proto::MessageType::EntityDestroy, [this](std::span<const std::uint8_t> payload) {
            proto::ByteReader reader(payload.data(), payload.size());
            if (const auto message = proto::EntityDestroyMessage::deserialize(reader))
                _world->destroy(*message);
        });
    _network->registerHandler(
        proto::MessageType::Snapshot, [this](std::span<const std::uint8_t> payload) {
            proto::ByteReader reader(payload.data(), payload.size());
            if (const auto message = proto::SnapshotMessage::deserialize(reader))
                _world->applySnapshot(*message);
        });
    _network->registerHandler(
        proto::MessageType::GameEvent, [this](std::span<const std::uint8_t> payload) {
            if (_frontend.page != ui::FrontendPage::Gameplay)
                return;
            proto::ByteReader reader(payload.data(), payload.size());
            const auto event = proto::GameEventMessage::deserialize(reader);
            if (!event)
                return;
            _sounds.push_back({event->event == proto::GameEventType::Shoot ? "shot" : "hit"});
        });
}
void ClientSession::lobby(const proto::LobbyStateMessage& state)
{
    const bool playing = state.phase == proto::LobbyPhase::Playing;
    if (!playing && _frontend.page == ui::FrontendPage::Gameplay)
        _world = std::make_unique<ClientWorld>();
    _frontend.page = playing ? ui::FrontendPage::Gameplay : ui::FrontendPage::Lobby;
    _frontend.leader = state.leaderId == _network->clientId();
    _frontend.mode = mode(state.kind);
    _frontend.level = state.level;
    _frontend.unlocked = state.unlocked;
    _frontend.levels = state.levelNames;
    if (_frontend.selectionPending && _pending.mode == _frontend.mode &&
        _pending.level == state.level)
        _frontend.selectionPending = false;
    if (!_frontend.leader || playing)
        _frontend.selectionPending = false;
    switch (state.phase) {
    case proto::LobbyPhase::LevelCleared:
        _frontend.message = "Level cleared. Choose the next stage.";
        break;
    case proto::LobbyPhase::CampaignCleared:
        _frontend.message = "Campaign complete.";
        break;
    case proto::LobbyPhase::GameOver:
        _frontend.message = "Game over. Choose a mode to play again.";
        break;
    default:
        _frontend.message.clear();
        break;
    }
}
void ClientSession::select(const ui::FrontendAction& action)
{
    if (action.mode == ui::FrontendMode::Duel ||
        (action.mode == ui::FrontendMode::Campaign &&
         (action.level >= _frontend.unlocked || action.level >= _frontend.levels.size())))
        return;
    if (action.mode == _frontend.mode && action.level == _frontend.level)
        return;
    _pending = action;
    _frontend.selectionPending = true;
    _waitingSince = std::chrono::steady_clock::now();
    _network->sendReliable(proto::MessageType::LobbySelect,
                           encode(proto::LobbySelectMessage{
                               kind(action.mode), static_cast<std::uint8_t>(action.level)}));
}
void ClientSession::handle(const ui::FrontendAction& action)
{
    switch (action.command) {
    case ui::FrontendCommand::Connect:
        if (_frontend.page == ui::FrontendPage::Connection ||
            _frontend.page == ui::FrontendPage::Error)
            connect(action.connection);
        break;
    case ui::FrontendCommand::Cancel:
    case ui::FrontendCommand::Disconnect:
        _network.reset();
        resetWorld();
        _frontend.page = ui::FrontendPage::Connection;
        _frontend.message = action.command == ui::FrontendCommand::Cancel ? "Connection cancelled."
                                                                          : "Disconnected.";
        break;
    case ui::FrontendCommand::Select:
        if (_network && _network->isConnected() && _frontend.page == ui::FrontendPage::Lobby &&
            _frontend.leader && !_frontend.selectionPending)
            select(action);
        break;
    case ui::FrontendCommand::Start:
        if (_network && _network->isConnected() && _frontend.page == ui::FrontendPage::Lobby &&
            _frontend.leader && !_frontend.selectionPending &&
            _frontend.mode != ui::FrontendMode::Duel &&
            (_frontend.mode != ui::FrontendMode::Campaign || _frontend.unlocked > 0))
            _network->sendReliable(proto::MessageType::LobbyStart,
                                   encode(proto::LobbyStartMessage{}));
        break;
    default:
        break;
    }
}
void ClientSession::update(float dt, const ui::PlayerActions& actions)
{
    _sounds.clear();
    if (!_network)
        return;
    _network->update();
    const auto elapsed = std::chrono::steady_clock::now() - _waitingSince;
    if (_frontend.selectionPending && elapsed > std::chrono::seconds(3)) {
        _frontend.selectionPending = false;
        _frontend.message = "Selection not confirmed by the server. Please retry.";
    }
    if (_frontend.page == ui::FrontendPage::Waiting && elapsed > std::chrono::seconds(10)) {
        _network.reset();
        resetWorld();
        _frontend.page = ui::FrontendPage::Error;
        _frontend.message = "The server did not send lobby information.";
    }
    if (_frontend.page != ui::FrontendPage::Gameplay || !_network->isConnected())
        return;
    _world->update(dt);
    std::uint8_t buttons = 0;
    for (std::size_t index = 0; index < actions.heldActions.size(); ++index)
        if (actions.heldActions[index])
            buttons = static_cast<std::uint8_t>(buttons | (1U << index));
    _network->sendUnreliable(proto::MessageType::Input,
                             encode(proto::InputMessage{_clientTick++, buttons}));
}
ui::RenderScene ClientSession::makeScene()
{
    ui::RenderScene scene;
    scene.frontend = _frontend;
    if (_frontend.page == ui::FrontendPage::Gameplay) {
        scene.clearColor = {20, 24, 32, 255};
        scene.worldSize = {1920, 1080};
        scene.background.enabled = true;
        scene.background.scrollSpeed = 40;
        addWorldSprites(scene, _world->registry(), _names);
    }
    return scene;
}
const std::vector<ui::SoundEvent>& ClientSession::sounds() const noexcept
{
    return _sounds;
}

} // namespace rtype::client
