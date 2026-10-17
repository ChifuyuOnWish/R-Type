#include "Gui.hpp"

#include <charconv>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

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

void configureDemo(int argc, char** argv, rtype::gui::GuiConfig& config,
                   rtype::gui::AudioSettings& settings)
{
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);
        if (argument == "--no-audio") {
            config.audioEnabled = false;
        } else if (argument == "--mute") {
            settings.muted = true;
        } else if (argument == "--volume" && index + 1 < argc) {
            const std::string_view value(argv[++index]);
            const auto result =
                std::from_chars(value.data(), value.data() + value.size(), settings.volume);
            if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
                throw std::invalid_argument("Volume must be a number between 0 and 1");
        } else {
            throw std::invalid_argument(
                "Usage: r-type_client [--no-audio] [--mute] [--volume 0..1]");
        }
    }
}

rtype::gui::RenderScene makeDemoScene(bool showEnemy)
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

    scene.overlayTexts.push_back(label("R-Type", 20.0F, 20.0F));
    return scene;
}

} // namespace

int main(int argc, char** argv)
{
    try {
        rtype::gui::GuiConfig config;
        rtype::gui::AudioSettings audioSettings{0.5F, false};
        configureDemo(argc, argv, config, audioSettings);
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
        auto& resources = gui.resources();
        resources.loadTexture("players", "player/ship-variants.gif");
        resources.loadTexture("player-weapons", "player/ship-and-effects.gif");
        resources.loadTexture("enemy", "enemy/red-fighter.gif");
        resources.loadTexture("enemy-weapons", "projectiles/red-beam-and-effects.gif");

        // Temporary local rendering fixture, replaced by client game state at integration.
        float elapsed = 0.0F;
        bool enemyRemovalReported = false;
        float fireCueRemaining = 0.0F;
        while (!gui.shouldClose()) {
            const auto actions = gui.pollActions();
            const bool firePressed =
                actions.pressedActions[static_cast<std::size_t>(rtype::gui::PlayerAction::Fire)];
            std::vector<rtype::gui::SoundEvent> soundEvents;
            if (firePressed) {
                fireCueRemaining = 0.25F;
                soundEvents.push_back({"demo-shot"});
            }
            if (elapsed >= 5.0F && !enemyRemovalReported) {
                enemyRemovalReported = true;
                soundEvents.push_back({"demo-hit"});
            }
            gui.playAudio(soundEvents);
            auto scene = makeDemoScene(elapsed < 5.0F);
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
