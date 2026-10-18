#include "ClientNetwork.hpp"
#include "ClientWorld.hpp"
#include "EntityType.hpp"
#include "Gui.hpp"
#include "MessageType.hpp"
#include "Messages.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

namespace net = rtype::network;
namespace proto = rtype::protocol;

using PlayerNames = std::unordered_map<std::uint32_t, std::string>;

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
    destination.pos.x -= destination.width / 2.0F;
    destination.pos.y -= destination.height / 2.0F;
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

void configureClient(int argc, char** argv, rtype::gui::GuiConfig& config,
                     rtype::gui::AudioSettings& settings, bool& overrideAudio,
                     NetworkOptions& network)
{
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);
        if (argument == "--no-audio") {
            config.audioEnabled = false;
        } else if (argument == "--mute") {
            settings.muted = true;
            overrideAudio = true;
        } else if (argument == "--settings" && index + 1 < argc) {
            config.settingsPath = argv[++index];
        } else if (argument == "--volume" && index + 1 < argc) {
            overrideAudio = true;
            const std::string_view value(argv[++index]);
            const auto result =
                std::from_chars(value.data(), value.data() + value.size(), settings.volume);
            if (result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
                !std::isfinite(settings.volume) || settings.volume < 0.0F || settings.volume > 1.0F)
                throw std::invalid_argument("Volume must be a number between 0 and 1");
        } else if (argument == "--connect" && index + 1 < argc) {
            parseEndpoint(argv[++index], network);
        } else if (argument == "--name" && index + 1 < argc) {
            network.name = argv[++index];
        } else {
            throw std::invalid_argument(
                "Usage: r-type_client [--no-audio] [--mute] [--volume 0..1] "
                "[--settings path] "
                "[--connect host:port] [--name NAME]");
        }
    }
}

void addWorldSprites(rtype::gui::RenderScene& scene, const rtype::client::ClientWorld& world,
                     const PlayerNames& playerNames)
{
    for (const auto& [entityId, entity] : world.entities()) {
        const auto type = static_cast<std::uint16_t>(entity.type);
        if (type >= static_cast<std::uint16_t>(proto::EntityType::Player1) &&
            type <= static_cast<std::uint16_t>(proto::EntityType::Player4)) {
            const std::uint32_t index =
                type - static_cast<std::uint16_t>(proto::EntityType::Player1);
            const float sourceY = 2.0F + static_cast<float>(index) * 17.0F;
            scene.sprites.push_back(sprite("players", {{67.0F, sourceY}, 32.0F, 15.0F},
                                           {{entity.x, entity.y}, 96.0F, 45.0F}, entityId, 10));
            const auto name = playerNames.find(index + 1);
            scene.sceneTexts.push_back(label(name != playerNames.end() ? name->second : "Player",
                                             entity.x - 48.0F, entity.y - 45.0F));
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

rtype::gui::RenderScene makeScene(const rtype::client::ClientWorld& world,
                                  const PlayerNames& playerNames, bool online)
{
    rtype::gui::RenderScene scene;
    scene.clearColor = {20, 24, 32, 255};
    scene.worldSize = {1920.0F, 1080.0F};
    scene.background.enabled = true;
    scene.background.scrollSpeed = 40.0F;
    addWorldSprites(scene, world, playerNames);
    scene.overlayTexts.push_back(label("R-Type", 20.0F, 20.0F));
    if (!online)
        scene.overlayTexts.push_back(
            label("Connect with --connect host:port --name NAME", 20.0F, 70.0F));
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
        config.width = 1920;
        config.height = 1080;
        rtype::gui::AudioSettings audioSettings{0.5F, false};
        rtype::gui::StatusFeedback feedback;
        NetworkOptions networkOptions;
        bool overrideAudio = false;
        configureClient(argc, argv, config, audioSettings, overrideAudio, networkOptions);
        rtype::gui::Gui gui;
        if (overrideAudio)
            gui.setAudioSettings(audioSettings);
        gui.open(config);
        if (config.audioEnabled && !gui.isAudioAvailable())
            std::cerr << gui.audio().getLastError() << '\n';
        bool shotLoaded = false;
        bool hitLoaded = false;
        if (gui.isAudioAvailable()) {
            shotLoaded = gui.audio().loadSound("shot", "audio/demo-shot.wav");
            if (!shotLoaded)
                std::cerr << gui.audio().getLastError() << '\n';
            hitLoaded = gui.audio().loadSound("hit", "audio/demo-hit.wav");
            if (!hitLoaded)
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
        std::uint32_t clientTick = 1;
        rtype::client::ClientWorld remoteWorld;
        PlayerNames playerNames;
        std::vector<rtype::gui::SoundEvent> soundEvents;

        if (networkOptions.enabled && feedback.status == rtype::gui::GuiStatus::None) {
            feedback.status = rtype::gui::GuiStatus::Connecting;
            feedback.message = "Connecting to " + networkOptions.host + ":" +
                               std::to_string(networkOptions.port) + "...";
            network.onConnected([&](net::PeerId id, const std::string& name) {
                playerNames[id] = name;
                feedback.status = rtype::gui::GuiStatus::None;
                feedback.message.clear();
                std::cout << "Connected as client " << id << '\n';
            });
            network.onConnectionFailed([&](net::ConnectFailure reason) {
                feedback.status = rtype::gui::GuiStatus::Rejected;
                feedback.message = "Connection failed: " + failureText(reason);
                std::cerr << "Connection failed: " << failureText(reason) << '\n';
            });
            network.onDisconnected([&](net::PeerId, net::DisconnectReason reason) {
                feedback.status = rtype::gui::GuiStatus::Disconnected;
                feedback.message = disconnectText(reason);
                std::cout << "Disconnected: " << disconnectText(reason) << '\n';
            });

            network.registerHandler(
                proto::MessageType::PlayerJoined, [&](std::span<const std::uint8_t> payload) {
                    proto::ByteReader reader(payload.data(), payload.size());
                    if (const auto joined = proto::PlayerJoinedMessage::deserialize(reader))
                        playerNames[joined->clientId] = joined->name;
                });
            network.registerHandler(
                proto::MessageType::PlayerLeft, [&](std::span<const std::uint8_t> payload) {
                    proto::ByteReader reader(payload.data(), payload.size());
                    if (const auto left = proto::PlayerLeftMessage::deserialize(reader))
                        playerNames.erase(left->clientId);
                });
            network.registerHandler(
                proto::MessageType::EntitySpawn, [&](std::span<const std::uint8_t> payload) {
                    proto::ByteReader reader(payload.data(), payload.size());
                    const auto spawn = proto::EntitySpawnMessage::deserialize(reader);
                    if (!spawn)
                        return;
                    remoteWorld.spawn(*spawn);
                });
            network.registerHandler(
                proto::MessageType::EntityDestroy, [&](std::span<const std::uint8_t> payload) {
                    proto::ByteReader reader(payload.data(), payload.size());
                    const auto destroy = proto::EntityDestroyMessage::deserialize(reader);
                    if (!destroy)
                        return;
                    remoteWorld.destroy(*destroy);
                });
            network.registerHandler(
                proto::MessageType::Snapshot, [&](std::span<const std::uint8_t> payload) {
                    proto::ByteReader reader(payload.data(), payload.size());
                    const auto snap = proto::SnapshotMessage::deserialize(reader);
                    if (!snap)
                        return;
                    remoteWorld.applySnapshot(*snap);
                });
            network.registerHandler(
                proto::MessageType::GameEvent, [&](std::span<const std::uint8_t> payload) {
                    proto::ByteReader reader(payload.data(), payload.size());
                    const auto event = proto::GameEventMessage::deserialize(reader);
                    if (!event || gui.isSettingsOpen() ||
                        feedback.status != rtype::gui::GuiStatus::None)
                        return;
                    const char* soundId = nullptr;
                    switch (event->event) {
                    case proto::GameEventType::Shoot:
                        if (shotLoaded)
                            soundId = "shot";
                        break;
                    case proto::GameEventType::Hit:
                        if (hitLoaded)
                            soundId = "hit";
                        break;
                    case proto::GameEventType::Kill:
                        if (hitLoaded)
                            soundId = "hit";
                        break;
                    case proto::GameEventType::PlayerDeath:
                        if (hitLoaded)
                            soundId = "hit";
                        break;
                    }
                    if (soundId != nullptr)
                        soundEvents.push_back({soundId});
                });
            if (const auto ec = network.connect(networkOptions.host, networkOptions.port,
                                                networkOptions.name)) {
                feedback.status = rtype::gui::GuiStatus::Error;
                feedback.message = "Connect error: " + ec.message();
                std::cerr << "connect: " << ec.message() << '\n';
            }
        }

        float fpsElapsed = 0.0F;
        unsigned fpsFrames = 0;
        unsigned displayedFps = 0;
        while (!gui.shouldClose()) {
            const auto actions = gui.pollActions();

            soundEvents.clear();
            network.update();
            gui.playAudio(soundEvents);
            if (network.isConnected())
                network.sendUnreliable(proto::MessageType::Input,
                                       encodeInput(clientTick, actions.heldActions));

            auto scene = makeScene(remoteWorld, playerNames, networkOptions.enabled);
            scene.feedback = feedback;
            if (displayedFps != 0) {
                auto fpsLabel = label("FPS: " + std::to_string(displayedFps), 20.0F, 52.0F);
                fpsLabel.fontSize = 14.0F;
                scene.overlayTexts.push_back(std::move(fpsLabel));
            }
            gui.render(scene);
            fpsElapsed += gui.getFrameTime();
            ++fpsFrames;
            if (fpsElapsed >= 0.5F) {
                displayedFps =
                    static_cast<unsigned>(std::lround(static_cast<float>(fpsFrames) / fpsElapsed));
                fpsElapsed = 0.0F;
                fpsFrames = 0;
            }
        }
        gui.close();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "r-type_client: " << error.what() << '\n';
        return 1;
    }
}
