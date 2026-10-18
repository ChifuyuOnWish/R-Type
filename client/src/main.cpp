#include "ClientNetwork.hpp"
#include "EntityType.hpp"
#include "Gui.hpp"
#include "MessageType.hpp"
#include "Messages.hpp"

#include <array>
#include <charconv>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

namespace net = rtype::network;
namespace proto = rtype::protocol;

struct NetworkOptions
{
    bool enabled = false;
    std::string host;
    std::uint16_t port = 0;
    std::string name = "Player";
};

rtype::gui::SpriteDraw sprite(const std::string& texture, rtype::gui::GuiRectangle source,
                              rtype::gui::GuiRectangle destination, std::uint32_t id, int layer)
{
    rtype::gui::SpriteDraw result;
    result.entityId = {id, 0};
    result.textureId = texture;
    result.sourceRect = source;
    result.destinationRect = destination;
    result.layer = layer;
    return result;
}

rtype::gui::TextDraw label(const std::string& text, float x, float y)
{
    rtype::gui::TextDraw result;
    result.text = text;
    result.position = {x, y};
    return result;
}

std::string actionNames(const std::array<bool, rtype::gui::playerActionCount>& actions)
{
    constexpr std::array names{"Up", "Down", "Left", "Right", "Fire"};
    std::string result;
    for (std::size_t index = 0; index < actions.size(); ++index) {
        if (!actions[index])
            continue;
        if (!result.empty())
            result += ' ';
        result += names[index];
    }
    return result.empty() ? "none" : result;
}

// "host:port" -> options.host / options.port
void parseEndpoint(std::string_view value, NetworkOptions& options)
{
    const auto colon = value.rfind(':');
    if (colon == std::string_view::npos || colon == 0 || colon + 1 >= value.size())
        throw std::invalid_argument("--connect expects host:port");
    const auto portText = value.substr(colon + 1);
    unsigned port = 0;
    const auto result = std::from_chars(portText.data(), portText.data() + portText.size(), port);
    if (result.ec != std::errc{} || result.ptr != portText.data() + portText.size() || port == 0 ||
        port > 65535)
        throw std::invalid_argument("--connect: invalid port");
    options.host = std::string(value.substr(0, colon));
    options.port = static_cast<std::uint16_t>(port);
    options.enabled = true;
}

rtype::gui::GuiStatus parseStatus(std::string_view value)
{
    if (value == "none")
        return rtype::gui::GuiStatus::None;
    if (value == "connecting")
        return rtype::gui::GuiStatus::Connecting;
    if (value == "rejected")
        return rtype::gui::GuiStatus::Rejected;
    if (value == "disconnected")
        return rtype::gui::GuiStatus::Disconnected;
    if (value == "error")
        return rtype::gui::GuiStatus::Error;
    throw std::invalid_argument("Status must be none, connecting, rejected, disconnected or error");
}

void configureDemo(int argc, char** argv, rtype::gui::GuiConfig& config,
                   rtype::gui::AudioSettings& settings, rtype::gui::StatusFeedback& feedback,
                   NetworkOptions& network)
{
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);
        if (argument == "--no-audio") {
            config.audioEnabled = false;
        } else if (argument == "--mute") {
            settings.muted = true;
        } else if (argument == "--status" && index + 1 < argc) {
            feedback.status = parseStatus(argv[++index]);
        } else if (argument == "--message" && index + 1 < argc) {
            feedback.message = argv[++index];
        } else if (argument == "--volume" && index + 1 < argc) {
            const std::string_view value(argv[++index]);
            const auto result =
                std::from_chars(value.data(), value.data() + value.size(), settings.volume);
            if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
                throw std::invalid_argument("Volume must be a number between 0 and 1");
        } else if (argument == "--connect" && index + 1 < argc) {
            parseEndpoint(argv[++index], network);
        } else if (argument == "--name" && index + 1 < argc) {
            network.name = argv[++index];
        } else {
            throw std::invalid_argument(
                "Usage: r-type_client [--no-audio] [--mute] [--volume 0..1] "
                "[--status none|connecting|rejected|disconnected|error] [--message text] "
                "[--connect host:port] [--name NAME]");
        }
    }
}

struct WorldEntity
{
    std::uint32_t entityId = 0;
    std::uint16_t entityType = 0;
    float x = 0.0F;
    float y = 0.0F;
};

using RemoteWorld = std::unordered_map<std::uint32_t, WorldEntity>;

void addWorldSprites(rtype::gui::RenderScene& scene, const RemoteWorld& world)
{
    for (const auto& [entityId, entity] : world) {
        const auto type = static_cast<std::uint16_t>(entity.entityType);
        if (type >= static_cast<std::uint16_t>(proto::EntityType::Player1) &&
            type <= static_cast<std::uint16_t>(proto::EntityType::Player4)) {
            const std::uint32_t index =
                type - static_cast<std::uint16_t>(proto::EntityType::Player1);
            const float sourceY = 2.0F + static_cast<float>(index) * 17.0F;
            scene.sprites.push_back(sprite("players", {{67.0F, sourceY}, 32.0F, 15.0F},
                                           {{entity.x, entity.y}, 96.0F, 45.0F}, entityId, 10));
        } else if (type == static_cast<std::uint16_t>(proto::EntityType::EnemyBasic)) {
            scene.sprites.push_back(sprite("enemy", {{5.0F, 6.0F}, 21.0F, 24.0F},
                                           {{entity.x, entity.y}, 63.0F, 72.0F}, entityId, 10));
        } else if (type == static_cast<std::uint16_t>(proto::EntityType::PlayerMissile)) {
            scene.sprites.push_back(sprite("player-weapons", {{215.0F, 85.0F}, 16.0F, 12.0F},
                                           {{entity.x, entity.y}, 48.0F, 36.0F}, entityId, 20));
        } else if (type == static_cast<std::uint16_t>(proto::EntityType::EnemyMissile)) {
            scene.sprites.push_back(sprite("enemy-weapons", {{136.0F, 6.0F}, 7.0F, 6.0F},
                                           {{entity.x, entity.y}, 21.0F, 18.0F}, entityId, 20));
        }
    }
}

rtype::gui::RenderScene makeDemoScene(const RemoteWorld& world, bool showEnemy)
{
    rtype::gui::RenderScene scene;
    scene.clearColor = {20, 24, 32, 255};
    scene.background.enabled = true;
    scene.background.scrollSpeed = 40.0F;

    for (std::uint32_t index = 0; index < 4; ++index) {
        const float y = 100.0F + static_cast<float>(index) * 130.0F;
        const float sourceY = 2.0F + static_cast<float>(index) * 17.0F;
        scene.sprites.push_back(sprite("players", {{67.0F, sourceY}, 32.0F, 15.0F},
                                       {{100.0F, y}, 96.0F, 45.0F}, index + 1, 10));
        scene.sceneTexts.push_back(label("P" + std::to_string(index + 1), 50.0F, y + 10.0F));
    }

    scene.sprites.push_back(sprite("player-weapons", {{215.0F, 85.0F}, 16.0F, 12.0F},
                                   {{350.0F, 115.0F}, 48.0F, 36.0F}, 10, 20));
    scene.sprites.push_back(sprite("enemy-weapons", {{136.0F, 6.0F}, 7.0F, 6.0F},
                                   {{700.0F, 315.0F}, 21.0F, 18.0F}, 11, 20));
    if (showEnemy)
        scene.sprites.push_back(sprite("enemy", {{5.0F, 6.0F}, 21.0F, 24.0F},
                                       {{850.0F, 300.0F}, 63.0F, 72.0F}, 12, 10));

    addWorldSprites(scene, world);
    scene.overlayTexts.push_back(label("R-Type", 20.0F, 20.0F));
    return scene;
}

std::string failureText(net::ConnectFailure reason)
{
    switch (reason) {
    case net::ConnectFailure::Full:
        return "server full";
    case net::ConnectFailure::BadVersion:
        return "bad protocol version";
    case net::ConnectFailure::Banned:
        return "banned";
    case net::ConnectFailure::Timeout:
        return "timeout";
    }
    return "unknown";
}

std::string disconnectText(net::DisconnectReason reason)
{
    switch (reason) {
    case net::DisconnectReason::Quit:
        return "disconnected";
    case net::DisconnectReason::Timeout:
        return "connection lost (timeout)";
    case net::DisconnectReason::Kicked:
        return "kicked";
    case net::DisconnectReason::ServerShutdown:
        return "server shut down";
    }
    return "disconnected";
}

std::vector<std::uint8_t> encodeInput(std::uint32_t& clientTick,
                                      const std::array<bool, rtype::gui::playerActionCount>& held)
{
    std::uint8_t mask = 0;
    for (std::size_t index = 0; index < held.size(); ++index)
        if (held[index])
            mask = static_cast<std::uint8_t>(mask | (1U << index));

    proto::InputMessage message{clientTick++, mask};
    proto::ByteWriter writer;
    if (!message.serialize(writer))
        throw std::runtime_error("failed to encode input message");
    return writer.data();
}

} // namespace

int main(int argc, char** argv)
{
    try {
        rtype::gui::GuiConfig config;
        rtype::gui::AudioSettings audioSettings{0.5F, false};
        rtype::gui::StatusFeedback feedback;
        NetworkOptions networkOptions;
        configureDemo(argc, argv, config, audioSettings, feedback, networkOptions);
        rtype::gui::Gui gui;
        gui.setAudioSettings(audioSettings);
        gui.open(config);
        if (config.audioEnabled && !gui.isAudioAvailable())
            std::cerr << gui.audio().getLastError() << '\n';
        if (gui.isAudioAvailable()) {
            if (!gui.audio().loadSound("demo-shot", "audio/demo-shot.wav"))
                std::cerr << gui.audio().getLastError() << '\n';
            if (!gui.audio().loadSound("demo-hit", "audio/demo-hit.wav"))
                std::cerr << gui.audio().getLastError() << '\n';
        }
        if (feedback.status == rtype::gui::GuiStatus::None) {
            try {
                auto& resources = gui.resources();
                resources.loadTexture("players", "player/ship-variants.gif");
                resources.loadTexture("player-weapons", "player/ship-and-effects.gif");
                resources.loadTexture("enemy", "enemy/red-fighter.gif");
                resources.loadTexture("enemy-weapons", "projectiles/red-beam-and-effects.gif");
            } catch (const std::exception& error) {
                feedback.status = rtype::gui::GuiStatus::Error;
                feedback.message = "Could not load visual resources: " + std::string(error.what());
                std::cerr << feedback.message << '\n';
            }
        }

        rtype::client::ClientNetwork network;
        std::string networkStatus = "offline";
        std::size_t messagesReceived = 0;
        std::size_t snapshotsReceived = 0;
        std::uint32_t clientTick = 0;
        std::uint32_t lastServerTick = 0;
        bool hasServerTick = false;
        RemoteWorld remoteWorld;

        if (networkOptions.enabled) {
            feedback.status = rtype::gui::GuiStatus::Connecting;
            feedback.message = "Connecting to " + networkOptions.host + ":" +
                               std::to_string(networkOptions.port) + "...";
            network.onConnected([&](net::PeerId id, const std::string&) {
                networkStatus = "connected (id " + std::to_string(id) + ")";
                feedback.status = rtype::gui::GuiStatus::None;
                feedback.message.clear();
                std::cout << "Connected as client " << id << '\n';
            });
            network.onConnectionFailed([&](net::ConnectFailure reason) {
                networkStatus = "connection failed: " + failureText(reason);
                feedback.status = rtype::gui::GuiStatus::Rejected;
                feedback.message = "Connection failed: " + failureText(reason);
                std::cerr << "Connection failed: " << failureText(reason) << '\n';
            });
            network.onDisconnected([&](net::PeerId, net::DisconnectReason reason) {
                networkStatus = disconnectText(reason);
                feedback.status = rtype::gui::GuiStatus::Disconnected;
                feedback.message = disconnectText(reason);
                std::cout << "Disconnected: " << disconnectText(reason) << '\n';
            });

            network.registerHandler(
                proto::MessageType::EntitySpawn, [&](std::span<const std::uint8_t> payload) {
                    ++messagesReceived;
                    proto::ByteReader reader(payload.data(), payload.size());
                    const auto spawn = proto::EntitySpawnMessage::deserialize(reader);
                    if (!spawn)
                        return;
                    auto& entity = remoteWorld[spawn->entityId];
                    entity.entityId = spawn->entityId;
                    entity.entityType = spawn->entityType;
                    entity.x = spawn->x.toFloat();
                    entity.y = spawn->y.toFloat();
                });
            network.registerHandler(
                proto::MessageType::EntityDestroy, [&](std::span<const std::uint8_t> payload) {
                    ++messagesReceived;
                    proto::ByteReader reader(payload.data(), payload.size());
                    const auto destroy = proto::EntityDestroyMessage::deserialize(reader);
                    if (!destroy)
                        return;
                    remoteWorld.erase(destroy->entityId);
                });
            network.registerHandler(
                proto::MessageType::Snapshot, [&](std::span<const std::uint8_t> payload) {
                    ++messagesReceived;
                    proto::ByteReader reader(payload.data(), payload.size());
                    const auto snap = proto::SnapshotMessage::deserialize(reader);
                    if (!snap)
                        return;
                    if (hasServerTick && snap->serverTick < lastServerTick)
                        return;
                    hasServerTick = true;
                    lastServerTick = snap->serverTick;
                    ++snapshotsReceived;
                    for (const auto& entry : snap->entries) {
                        auto it = remoteWorld.find(entry.entityId);
                        WorldEntity entity;
                        entity.entityId = entry.entityId;
                        if (it != remoteWorld.end())
                            entity.entityType = it->second.entityType;
                        entity.x = entry.x.toFloat();
                        entity.y = entry.y.toFloat();
                        remoteWorld[entry.entityId] = entity;
                    }
                });
            for (const auto type : {proto::MessageType::PlayerJoined,
                                    proto::MessageType::PlayerLeft, proto::MessageType::GameEvent})
                network.registerHandler(type,
                                        [&](std::span<const std::uint8_t>) { ++messagesReceived; });

            if (const auto ec = network.connect(networkOptions.host, networkOptions.port,
                                                networkOptions.name)) {
                networkStatus = "connect error: " + ec.message();
                feedback.status = rtype::gui::GuiStatus::Error;
                feedback.message = "Connect error: " + ec.message();
                std::cerr << "connect: " << ec.message() << '\n';
            } else {
                networkStatus = "connecting...";
            }
        }

        float elapsed = 0.0F;
        bool enemyRemovalReported = false;
        float fireCueRemaining = 0.0F;
        while (!gui.shouldClose()) {
            const auto actions = gui.pollActions();

            network.update();
            if (network.isConnected())
                network.sendUnreliable(proto::MessageType::Input,
                                       encodeInput(clientTick, actions.heldActions));

            const bool showGame = feedback.status == rtype::gui::GuiStatus::None;
            const bool firePressed =
                showGame &&
                actions.pressedActions[static_cast<std::size_t>(rtype::gui::PlayerAction::Fire)];
            std::vector<rtype::gui::SoundEvent> soundEvents;
            if (firePressed) {
                fireCueRemaining = 0.25F;
                soundEvents.push_back({"demo-shot"});
            }
            if (showGame && elapsed >= 5.0F && !enemyRemovalReported) {
                enemyRemovalReported = true;
                soundEvents.push_back({"demo-hit"});
            }
            gui.playAudio(soundEvents);
            auto scene = makeDemoScene(remoteWorld, elapsed < 5.0F);
            scene.feedback = feedback;
            scene.overlayTexts.push_back(label("Arrows: move | Space: fire", 20.0F, 640.0F));
            scene.overlayTexts.push_back(
                label("Held: " + actionNames(actions.heldActions), 20.0F, 670.0F));
            const auto pressed = actionNames(actions.pressedActions);
            const auto released = actionNames(actions.releasedActions);
            if (pressed != "none")
                std::cout << "Pressed: " << pressed << '\n';
            if (released != "none")
                std::cout << "Released: " << released << '\n';
            const std::string audioStatus = !gui.isAudioAvailable() ? "unavailable"
                                            : audioSettings.muted   ? "muted"
                                                                    : "enabled";
            scene.overlayTexts.push_back(label("Audio: " + audioStatus, 20.0F, 50.0F));
            if (networkOptions.enabled) {
                scene.overlayTexts.push_back(label("Network: " + networkStatus, 20.0F, 80.0F));
                scene.overlayTexts.push_back(
                    label("Msgs: " + std::to_string(messagesReceived) +
                              " | Snapshots: " + std::to_string(snapshotsReceived) +
                              " | Dropped: " + std::to_string(network.droppedPackets()),
                          20.0F, 110.0F));
            }
            if (fireCueRemaining > 0.0F)
                scene.overlayTexts.push_back(label("Fire input", 350.0F, 80.0F));
            if (enemyRemovalReported)
                scene.overlayTexts.push_back(label("Enemy removed", 850.0F, 270.0F));
            gui.render(scene);
            fireCueRemaining -= gui.getFrameTime();
            elapsed += gui.getFrameTime();
        }
        gui.close();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "r-type_client: " << error.what() << '\n';
        return 1;
    }
}
