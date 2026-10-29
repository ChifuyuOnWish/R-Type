#include "Gui.hpp"

#include "Renderer.hpp"
#include "SettingsStore.hpp"
#include "StatusPresenter.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <raylib.h>
#include <stdexcept>
#include <utility>

namespace rtype::gui {
namespace {

bool contains(const GuiRectangle& rectangle, GuiPosition position)
{
    return position.x >= rectangle.pos.x && position.x < rectangle.pos.x + rectangle.width &&
           position.y >= rectangle.pos.y && position.y < rectangle.pos.y + rectangle.height;
}

GuiRectangle settingsButton(GuiVector2 viewport, float scale)
{
    const float side = std::max(36.0F, 48.0F * scale);
    return {{std::max(12.0F, viewport.x - side - 12.0F), 12.0F}, side, side};
}

RenderScene withSettingsButton(RenderScene scene, GuiVector2 viewport, float scale,
                               const ResourceManager& resources)
{
    const auto button = settingsButton(viewport, scale);
    if (resources.hasTexture("ui-settings")) {
        SpriteDraw sprite;
        sprite.textureId = "ui-settings";
        const auto size = resources.getTextureSize(sprite.textureId);
        sprite.sourceRect = {{0, 0}, size.x, size.y};
        sprite.destinationRect = button;
        scene.sprites.push_back(std::move(sprite));
    } else {
        scene.rectangles.push_back({button, {255, 255, 255, 255}});
        TextDraw label;
        label.text = "*";
        label.fontSize = 24.0F * scale;
        label.color = {20, 20, 20, 255};
        label.position = {button.pos.x + button.width * 0.35F,
                          button.pos.y + button.height * 0.25F};
        scene.overlayTexts.push_back(std::move(label));
    }
    return scene;
}

RenderScene withUiFont(RenderScene scene, const std::string& font)
{
    for (auto* texts : {&scene.sceneTexts, &scene.overlayTexts})
        for (auto& text : *texts)
            if (text.fontId.empty())
                text.fontId = font;
    return scene;
}

void fitWorld(RenderScene& scene, GuiVector2 viewport)
{
    const auto world = scene.worldSize;
    if (world.x == 0.0F && world.y == 0.0F)
        return;
    if (!std::isfinite(world.x) || !std::isfinite(world.y) || world.x <= 0.0F || world.y <= 0.0F)
        throw std::invalid_argument("World dimensions must be finite and positive");
    if (viewport.x <= 0.0F || viewport.y <= 0.0F)
        return;
    const float scale = std::min(viewport.x / world.x, viewport.y / world.y);
    const GuiPosition offset{(viewport.x - world.x * scale) / 2.0F,
                             (viewport.y - world.y * scale) / 2.0F};
    for (auto& sprite : scene.sprites) {
        auto& destination = sprite.destinationRect;
        destination.pos = {offset.x + destination.pos.x * scale,
                           offset.y + destination.pos.y * scale};
        destination.width *= scale;
        destination.height *= scale;
        sprite.origin.x *= scale;
        sprite.origin.y *= scale;
    }
    for (auto& text : scene.sceneTexts) {
        text.position = {offset.x + text.position.x * scale, offset.y + text.position.y * scale};
        text.fontSize *= scale;
        text.spacing *= scale;
    }
}

GuiColor sceneTint(const VisualSettings& visual)
{
    constexpr std::array<GuiColor, 5> colors{{{0, 0, 0, 0},
                                              {255, 80, 80, 255},
                                              {80, 255, 120, 255},
                                              {80, 140, 255, 255},
                                              {255, 190, 80, 255}}};
    auto color = colors[static_cast<std::size_t>(visual.tint)];
    color.alpha = visual.tint == TintMode::Off
                      ? 0
                      : static_cast<std::uint8_t>(std::round(visual.tintOpacity * 255.0F));
    return color;
}

} // namespace

Gui::Gui(std::filesystem::path assetRoot)
    : _resources(std::move(assetRoot)), _audio(_resources.getAssetRoot())
{
}

Gui::~Gui()
{
    close();
}

void Gui::applyPreferences(const GuiPreferences& preferences)
{
    SettingsStore::validate(preferences);
    _input.setKeyBindings(preferences.keyboard);
    _audio.setSettings(preferences.audio);
    _preferences = preferences;
}

void Gui::open(const GuiConfig& config)
{
    _window.open(config);
    _frontend.reset();
    _menu = SettingsMenu{};
    const auto root = _resources.getAssetRoot();
    _uiFont.clear();
    const auto fontPath = root / "fonts/holidays-homework.ttf";
    if (std::filesystem::is_regular_file(fontPath)) {
        _resources.loadFont("ui-handwriting", "fonts/holidays-homework.ttf", 96);
        _uiFont = "ui-handwriting";
        _frontend.setFont(_uiFont);
    }
    _frontend.setTextMeasure([this](const std::string& value, float size) {
        return _resources.measureText(_uiFont, value, size).x;
    });
    for (const char* name : {"title",
                             "settings",
                             "input",
                             "button",
                             "campaign",
                             "duel",
                             "infinite",
                             "name-field",
                             "ip-field",
                             "port-field",
                             "connect",
                             "retry",
                             "cancel",
                             "back",
                             "start",
                             "previous",
                             "next",
                             "disconnect",
                             "quit",
                             "background",
                             "settings-background",
                             "reset",
                             "save",
                             "settings-display",
                             "settings-audio",
                             "settings-controls",
                             "settings-accessibility"}) {
        const auto path = std::filesystem::path("ui") / (std::string(name) + ".png");
        if (std::filesystem::is_regular_file(root / path)) {
            const auto id = "ui-" + std::string(name);
            _resources.loadTexture(id, path);
            _frontend.setArtwork(name, _resources.getTextureSize(id));
            _menu.setArtwork(name, _resources.getTextureSize(id));
        }
    }
    _menu.setTextMeasure([this](const std::string& value, float size) {
        return _resources.measureText(_uiFont, value, size).x;
    });
    _fpsElapsed = 0;
    _fpsFrames = 0;
    _displayedFps = 0;
    _quitRequested = false;
    _background = {};
    _input.reset();
    _menu.close();
    _waitForNeutral = true;
    _settingsPath = config.settingsPath.empty() ? SettingsStore::defaultPath()
                                                : std::filesystem::absolute(config.settingsPath);
    auto loaded = _preferences;
    (void)SettingsStore::load(_settingsPath, loaded, _settingsNotice);
    if (_keyOverride)
        loaded.keyboard = _preferences.keyboard;
    if (_audioOverride)
        loaded.audio = _preferences.audio;
    applyPreferences(loaded);
    if (!_settingsNotice.empty()) {
        std::cerr << "Settings: " << _settingsNotice << '\n';
        _settingsNotice = "Saved settings could not be loaded. See terminal.";
    }
    if (config.audioEnabled)
        (void)_audio.open();
}

void Gui::close() noexcept
{
    _menu.close();
    _input.reset();
    _audio.close();
    _resources.clear();
    _window.close();
}

bool Gui::shouldClose() const
{
    return _quitRequested || _window.shouldClose();
}

void Gui::setKeyBindings(const KeyBindings& bindings)
{
    _input.setKeyBindings(bindings);
    _preferences.keyboard = bindings;
    _keyOverride = true;
    _waitForNeutral = true;
}

PlayerActions Gui::pollActions()
{
    if (!_window.isOpen())
        throw std::logic_error("Cannot poll actions before opening the GUI");
    auto frame = _reader.poll();
    if (_frontend.active() && !_menu.isOpen()) {
        _waitForNeutral = true;
        auto actions = _input.map(KeyboardState{false});
        actions.frontend =
            _frontend.handle(frame, _window.getSize(), _preferences.visuals.textScale);
        if (actions.frontend.command == FrontendCommand::Quit)
            _quitRequested = true;
        if (_frontend.takeSettingsRequest() || frame.settingsShortcut) {
            _menu.open(_preferences);
            _menu.setNotice(_settingsNotice);
        }
        return actions;
    }
    bool handledToggle = false;
    if (frame.toggleSettings) {
        handledToggle = true;
        if (!_menu.isOpen()) {
            _menu.open(_preferences);
            _menu.setNotice(_settingsNotice);
        } else {
            frame.menu.back = true;
            frame.menu.cancelCapture = true;
            (void)_menu.handle(frame.menu);
        }
        _waitForNeutral = true;
    }
    if (!_menu.isOpen() && !handledToggle && frame.mouseClick &&
        contains(settingsButton(_window.getSize(), _preferences.visuals.textScale), frame.mouse)) {
        _menu.open(_preferences);
        _menu.setNotice(_settingsNotice);
        _waitForNeutral = true;
        handledToggle = true;
    }
    if (_menu.isOpen()) {
        if (!handledToggle) {
            if (frame.wheel != 0.0F)
                frame.menu.vertical = frame.wheel > 0.0F ? -1 : 1;
            if (frame.mouseClick && !_menu.isCapturing()) {
                for (const auto& row : _menu.rows(_window.getSize())) {
                    if (!contains(row.bounds, frame.mouse))
                        continue;
                    _menu.select(row.index);
                    frame.menu.activate = true;
                    if (row.adjustable) {
                        if (frame.mouse.x >= row.bounds.pos.x + row.bounds.width - 40.0F)
                            frame.menu.horizontal = 1;
                        else if (frame.mouse.x >= row.bounds.pos.x + row.bounds.width * 0.60F &&
                                 frame.mouse.x <
                                     row.bounds.pos.x + row.bounds.width * 0.60F + 36.0F)
                            frame.menu.horizontal = -1;
                    }
                    break;
                }
            }
            const auto result = _menu.handle(frame.menu);
            if (result == MenuResult::Apply) {
                applyPreferences(_menu.draft());
                if (SettingsStore::save(_settingsPath, _preferences, _settingsNotice)) {
                    _settingsNotice = "Settings applied and saved.";
                } else {
                    std::cerr << "Settings save failed: " << _settingsNotice << '\n';
                    _settingsNotice = "Settings applied. Save failed; see terminal.";
                }
                _menu.setNotice(_settingsNotice);
            } else if (result == MenuResult::Quit) {
                _quitRequested = true;
            }
        }
        _waitForNeutral = true;
        return _input.map(KeyboardState{false});
    }
    if (handledToggle) {
        _waitForNeutral = true;
        return _input.map(KeyboardState{false});
    }
    if (_waitForNeutral) {
        if (std::any_of(frame.keyboard.heldKeys.begin(), frame.keyboard.heldKeys.end(),
                        [](bool held) { return held; }))
            return _input.map(KeyboardState{false});
        _waitForNeutral = false;
    }
    return _input.map(frame.keyboard);
}

bool Gui::isSettingsOpen() const noexcept
{
    return _menu.isOpen();
}

const GuiPreferences& Gui::getPreferences() const noexcept
{
    return _preferences;
}

void Gui::render(const RenderScene& scene)
{
    if (!_window.isOpen())
        throw std::logic_error("Cannot render before opening the GUI");
    _fpsElapsed += _window.getFrameTime();
    ++_fpsFrames;
    if (_fpsElapsed >= 0.5F) {
        _displayedFps =
            static_cast<unsigned>(std::lround(static_cast<float>(_fpsFrames) / _fpsElapsed));
        _fpsElapsed = 0;
        _fpsFrames = 0;
    }
    const auto draw = [this](RenderScene presented, std::span<const GuiPosition> stars = {}) {
        const auto& visual = _menu.isOpen() ? _menu.draft().visuals : _preferences.visuals;
        if (visual.showFps && _displayedFps != 0) {
            TextDraw counter;
            counter.fontId = _uiFont;
            counter.text = "FPS: " + std::to_string(_displayedFps);
            counter.fontSize = 18.0F * visual.textScale;
            counter.position = {12, 8};
            counter.color = {20, 20, 20, 255};
            const auto size = _resources.measureText(_uiFont, counter.text, counter.fontSize);
            presented.rectangles.push_back(
                {{{8, 4}, size.x + 8, size.y + 8}, {255, 255, 255, 255}});
            presented.overlayTexts.push_back(std::move(counter));
        }
        Renderer(_resources).render(presented, stars);
    };
    _frontend.setState(scene.frontend);
    if (_menu.isOpen()) {
        draw(withUiFont(_menu.makeScene(_window.getSize()), _uiFont));
        return;
    }
    if (_frontend.active()) {
        draw(_frontend.makeScene(_window.getSize(), _preferences.visuals.textScale));
        return;
    }
    const auto& visual = _preferences.visuals;
    if (scene.feedback.status != GuiStatus::None) {
        const auto statusScene =
            StatusPresenter::makeScene(scene.feedback, _window.getSize(), visual.textScale);
        draw(withUiFont(
            withSettingsButton(statusScene, _window.getSize(), visual.textScale, _resources),
            _uiFont));
        return;
    }
    if (visual.backgroundMotion)
        _background.update(scene.background, _window.getFrameTime());
    const auto stars = scene.background.enabled ? _background.getStars(_window.getSize())
                                                : std::vector<GuiPosition>{};
    auto presented = scene;
    fitWorld(presented, _window.getSize());
    for (auto* texts : {&presented.sceneTexts, &presented.overlayTexts}) {
        for (auto& text : *texts) {
            text.fontSize *= visual.textScale;
            text.spacing *= visual.textScale;
        }
    }
    presented =
        withSettingsButton(std::move(presented), _window.getSize(), visual.textScale, _resources);
    presented = withUiFont(std::move(presented), _uiFont);
    presented.sceneTint = sceneTint(visual);
    draw(std::move(presented), stars);
}

void Gui::setAudioSettings(const AudioSettings& settings)
{
    _audio.setSettings(settings);
    _preferences.audio = settings;
    _audioOverride = true;
}

bool Gui::isAudioAvailable() const noexcept
{
    return _audio.isAvailable();
}

void Gui::playAudio(std::span<const SoundEvent> events)
{
    if (!_window.isOpen())
        throw std::logic_error("Cannot play audio before opening the GUI");
    (void)_audio.play(events);
}

float Gui::getFrameTime() const
{
    return _window.getFrameTime();
}

AudioSystem& Gui::audio() noexcept
{
    return _audio;
}
const AudioSystem& Gui::audio() const noexcept
{
    return _audio;
}
ResourceManager& Gui::resources() noexcept
{
    return _resources;
}
const ResourceManager& Gui::resources() const noexcept
{
    return _resources;
}

} // namespace rtype::gui
