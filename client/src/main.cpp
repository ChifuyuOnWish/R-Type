#include "ClientSession.hpp"
#include "Gui.hpp"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

struct NetworkOptions
{
    bool enabled = false;
    std::string host;
    std::uint16_t port = 0;
    std::string name = "Player";
};

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

} // namespace

int main(int argc, char** argv)
{
    try {
        rtype::gui::GuiConfig config;
        config.width = 1920;
        config.height = 1080;
        rtype::gui::AudioSettings audioSettings{0.5F, false};
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
        {
            try {
                auto& resources = gui.resources();
                resources.loadTexture("players", "player/ship-variants.gif");
                resources.loadTexture("player-weapons", "player/ship-and-effects.gif");
                resources.loadTexture("enemy", "enemy/red-fighter.gif");
                resources.loadTexture("enemy-weapons", "projectiles/red-beam-and-effects.gif");
            } catch (const std::exception& error) {
                throw std::runtime_error("Could not load visual resources: " +
                                         std::string(error.what()));
            }
        }

        rtype::gui::ConnectionDetails connection;
        connection.name = networkOptions.name;
        if (networkOptions.enabled) {
            connection.host = networkOptions.host;
            connection.port = networkOptions.port;
        }
        rtype::client::ClientSession session(connection);
        if (networkOptions.enabled) {
            rtype::gui::FrontendAction action;
            action.command = rtype::gui::FrontendCommand::Connect;
            action.connection = connection;
            session.handle(action);
        }
        gui.render(session.makeScene());

        while (!gui.shouldClose()) {
            const auto actions = gui.pollActions();

            session.handle(actions.frontend);
            session.update(gui.getFrameTime(), actions);
            if (!gui.isSettingsOpen()) {
                std::vector<rtype::gui::SoundEvent> sounds;
                for (const auto& event : session.sounds())
                    if ((event.soundId == "shot" && shotLoaded) ||
                        (event.soundId == "hit" && hitLoaded))
                        sounds.push_back(event);
                gui.playAudio(sounds);
            }
            auto scene = session.makeScene();
            gui.render(scene);
        }
        gui.close();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "r-type_client: " << error.what() << '\n';
        return 1;
    }
}
