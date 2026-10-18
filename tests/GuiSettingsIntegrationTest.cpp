#include "Gui.hpp"
#include "Renderer.hpp"
#include "SettingsStore.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <raylib.h>
#include <stdexcept>

using namespace rtype::gui;
namespace {
std::array<bool, 512> keys{}, presses{};
bool ready = false, focused = true, clicked = false;
Vector2 mouse{};
float wheel = 0;
int exitKey = -1;
int aspectNumerator = 0, aspectDenominator = 0;
int screenWidth = 1280, screenHeight = 720;
RenderScene rendered;
void check(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}
PlayerActions poll(Gui& gui)
{
    auto result = gui.pollActions();
    presses = {};
    clicked = false;
    wheel = 0;
    return result;
}
void press(Gui& gui, int key)
{
    presses[static_cast<std::size_t>(key)] = true;
    (void)poll(gui);
}
constexpr auto fire = static_cast<std::size_t>(PlayerAction::Fire);
} // namespace

// Only the native boundary is replaced; Gui, input and menu orchestration are real.
extern "C" {
GLFWwindow* glfwGetCurrentContext()
{
    static int token;
    return reinterpret_cast<GLFWwindow*>(&token);
}
void glfwSetWindowAspectRatio(GLFWwindow*, int numerator, int denominator)
{
    aspectNumerator = numerator;
    aspectDenominator = denominator;
}
Font GetFontDefault()
{
    return {};
}
Vector2 MeasureTextEx(Font, const char* text, float size, float spacing)
{
    return {static_cast<float>(std::string(text).size()) * (size * 0.5F + spacing), size};
}
void SetConfigFlags(unsigned int) {}
void InitWindow(int, int, const char*)
{
    ready = true;
}
bool IsWindowReady()
{
    return ready;
}
void CloseWindow()
{
    ready = false;
}
void SetExitKey(int key)
{
    exitKey = key;
}
void SetWindowMinSize(int, int) {}
void SetTargetFPS(int) {}
bool WindowShouldClose()
{
    return false;
}
int GetScreenWidth()
{
    return screenWidth;
}
int GetScreenHeight()
{
    return screenHeight;
}
bool IsWindowResized()
{
    return false;
}
float GetFrameTime()
{
    return 1.0F / 60.0F;
}
bool IsWindowFocused()
{
    return focused;
}
bool IsKeyDown(int key)
{
    return keys.at(static_cast<std::size_t>(key));
}
bool IsKeyPressed(int key)
{
    return presses.at(static_cast<std::size_t>(key));
}
bool IsKeyPressedRepeat(int)
{
    return false;
}
Vector2 GetMousePosition()
{
    return mouse;
}
bool IsMouseButtonPressed(int)
{
    return clicked;
}
float GetMouseWheelMove()
{
    return wheel;
}
}
namespace rtype::gui {
struct ResourceManager::Impl
{
    std::filesystem::path root;
};
ResourceManager::ResourceManager(std::filesystem::path path) : _impl(std::make_unique<Impl>())
{
    _impl->root = std::move(path);
}
ResourceManager::~ResourceManager() = default;
const std::filesystem::path& ResourceManager::getAssetRoot() const noexcept
{
    return _impl->root;
}
void ResourceManager::clear() noexcept {}
struct AudioSystem::Impl
{
    AudioSettings settings;
};
AudioSystem::AudioSystem(std::filesystem::path) : _impl(std::make_unique<Impl>()) {}
AudioSystem::~AudioSystem() = default;
bool AudioSystem::open()
{
    return false;
}
void AudioSystem::close() noexcept {}
bool AudioSystem::isAvailable() const noexcept
{
    return false;
}
void AudioSystem::setSettings(const AudioSettings& settings)
{
    _impl->settings = settings;
}
std::size_t AudioSystem::play(std::span<const SoundEvent>)
{
    return 0;
}
Renderer::Renderer(const ResourceManager& resources) : _resources(resources) {}
void Renderer::render(const RenderScene& scene, std::span<const GuiPosition>) const
{
    rendered = scene;
}
} // namespace rtype::gui

int main()
{
    const auto root = std::filesystem::temp_directory_path() /
                      ("rtype-menu-integration-" +
                       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        GuiConfig config;
        config.audioEnabled = false;
        config.settingsPath = (root / "settings.conf").string();
        Gui gui;
        gui.open(config);
        check(exitKey == KEY_NULL, "Escape reserved for menu");
        (void)poll(gui);
        keys[KEY_SPACE] = true;
        check(poll(gui).pressedActions[fire], "Gameplay before menu");
        presses[KEY_F1] = true;
        auto actions = poll(gui);
        check(gui.isSettingsOpen() && actions.releasedActions[fire] && !actions.heldActions[fire],
              "Opening menu releases gameplay actions");
        gui.render({});
        check(rendered.overlayTexts[0].text == "Settings", "Menu rendered");
        press(gui, KEY_ESCAPE);
        check(!gui.isSettingsOpen() && !poll(gui).heldActions[fire],
              "Held fire blocked after closing");
        keys = {};
        (void)poll(gui);
        keys[KEY_SPACE] = true;
        check(poll(gui).pressedActions[fire], "Gameplay resumes after release");
        keys = {};
        (void)poll(gui);
        press(gui, KEY_F1);
        check(gui.isSettingsOpen(), "Settings reopen");
        press(gui, KEY_DOWN);
        press(gui, KEY_ENTER);
        check(!gui.getPreferences().audio.muted, "Draft has not been applied");
        // Mouse wheel reaches Apply and save; keyboard activates it.
        for (int index = 0; index < 11; ++index) {
            wheel = -1;
            (void)poll(gui);
        }
        press(gui, KEY_ENTER);
        check(gui.getPreferences().audio.muted, "Draft applied");
        GuiPreferences saved;
        std::string error;
        check(SettingsStore::load(config.settingsPath, saved, error) && saved.audio.muted,
              "Applied preferences persisted");
        press(gui, KEY_ESCAPE);
        gui.close();
        gui.open(config);
        check(gui.getPreferences().audio.muted, "Saved preferences restored on reopen");
        (void)poll(gui);
        mouse = {1200, 25};
        clicked = true;
        (void)poll(gui);
        check(gui.isSettingsOpen(), "Mouse settings button");
        // Set a red scene tint, save it, and return to the scene.
        for (int index = 0; index < 4; ++index)
            press(gui, KEY_DOWN);
        press(gui, KEY_RIGHT);
        for (int index = 0; index < 8; ++index)
            press(gui, KEY_DOWN);
        press(gui, KEY_ENTER);
        press(gui, KEY_ESCAPE);
        gui.render({});
        check(rendered.sceneTint.red == 255 && rendered.sceneTint.alpha == 38,
              "Tint preference reaches the renderer");
        check(!rendered.overlayTexts.empty(), "Settings button stays available");
        press(gui, KEY_F1);
        for (int index = 0; index < 3; ++index)
            press(gui, KEY_DOWN);
        press(gui, KEY_RIGHT);
        for (int index = 0; index < 9; ++index)
            press(gui, KEY_DOWN);
        press(gui, KEY_ENTER);
        press(gui, KEY_ESCAPE);
        RenderScene textScene;
        TextDraw label;
        label.text = "HUD";
        label.fontSize = 20;
        textScene.overlayTexts.push_back(label);
        gui.render(textScene);
        check(rendered.overlayTexts[0].fontSize == 25.0F,
              "Text scale applies to client-supplied HUD text");
        RenderScene worldScene;
        worldScene.worldSize = {1920.0F, 1080.0F};
        SpriteDraw ship;
        ship.destinationRect = {{300.0F, 600.0F}, 96.0F, 45.0F};
        ship.sourceRect = {{67.0F, 2.0F}, 32.0F, 15.0F};
        ship.origin = {12.0F, 6.0F};
        worldScene.sprites.push_back(ship);
        worldScene.sceneTexts.push_back(label);
        worldScene.overlayTexts.push_back(label);
        gui.render(worldScene);
        const auto& fitted = rendered.sprites.front();
        check(fitted.destinationRect.pos.x == 200.0F && fitted.destinationRect.pos.y == 400.0F &&
                  fitted.destinationRect.width == 64.0F && fitted.destinationRect.height == 30.0F,
              "World fits the window without changing aspect ratio");
        check(fitted.sourceRect.width == 32.0F && fitted.origin.x == 8.0F,
              "Texture regions stay unchanged and origins scale with sprites");
        check(rendered.overlayTexts.front().fontSize == 25.0F,
              "HUD keeps its size independently of world scaling");
        check(worldScene.sprites.front().destinationRect.width == 96.0F,
              "Rendering does not mutate the submitted scene");
        screenWidth = 1600;
        screenHeight = 1000;
        gui.render(worldScene);
        check(std::abs(rendered.sprites.front().destinationRect.pos.x - 250.0F) < 0.01F &&
                  std::abs(rendered.sprites.front().destinationRect.pos.y - 550.0F) < 0.01F,
              "A taller viewport centers the scaled world vertically");
        screenWidth = 1280;
        screenHeight = 720;
        check(aspectNumerator == 16 && aspectDenominator == 9,
              "The native window is configured once to preserve the 16:9 aspect ratio");
        press(gui, KEY_F1);
        press(gui, KEY_UP);
        press(gui, KEY_ENTER);
        check(gui.shouldClose(), "Explicit menu quit");
        gui.close();
        std::filesystem::remove_all(root);
        return 0;
    } catch (const std::exception& error) {
        std::filesystem::remove_all(root);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
