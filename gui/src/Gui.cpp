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
    const float width = std::min(112.0F * scale, std::max(1.0F, viewport.x - 24.0F));
    return {{std::max(12.0F, viewport.x - width - 12.0F), 12.0F}, width, 44.0F * scale};
}

RenderScene withSettingsButton(RenderScene scene, GuiVector2 viewport, float scale)
{
    const auto button = settingsButton(viewport, scale);
    scene.rectangles.push_back({button, {65, 79, 102, 255}});
    const float border = scale;
    scene.rectangles.push_back({{{button.pos.x + border, button.pos.y + border},
                                 button.width - 2.0F * border,
                                 button.height - 2.0F * border},
                                {25, 33, 47, 255}});
    const auto addLabel = [&](const char* value, float size, float offset, GuiColor color) {
        TextDraw label;
        label.text = value;
        label.fontSize = size * scale;
        label.spacing = scale;
        const auto measured = MeasureTextEx(GetFontDefault(), value, label.fontSize, label.spacing);
        label.position = {button.pos.x + (button.width - measured.x) / 2.0F,
                          button.pos.y + offset * scale};
        label.color = color;
        scene.overlayTexts.push_back(std::move(label));
    };
    addLabel("Settings", 14.0F, 7.0F, {240, 244, 250, 255});
    addLabel("F1 / Esc", 10.0F, 27.0F, {172, 185, 206, 255});
    return scene;
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
    _menu.setTextMeasure([](const std::string& value, float size) {
        return MeasureTextEx(GetFontDefault(), value.c_str(), size, 1.0F).x;
    });
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
                    if (row.index <= 5) {
                        if (frame.mouse.x >= row.bounds.pos.x + row.bounds.width - 40.0F)
                            frame.menu.horizontal = 1;
                        else if (frame.mouse.x >= row.bounds.pos.x + row.bounds.width - 80.0F)
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
    if (_menu.isOpen()) {
        Renderer(_resources).render(_menu.makeScene(_window.getSize()));
        return;
    }
    const auto& visual = _preferences.visuals;
    if (scene.feedback.status != GuiStatus::None) {
        const auto statusScene =
            StatusPresenter::makeScene(scene.feedback, _window.getSize(), visual.textScale);
        Renderer(_resources)
            .render(withSettingsButton(statusScene, _window.getSize(), visual.textScale));
        return;
    }
    if (visual.backgroundMotion)
        _background.update(scene.background, _window.getFrameTime());
    const auto stars = scene.background.enabled ? _background.getStars(_window.getSize())
                                                : std::vector<GuiPosition>{};
    auto presented = scene;
    for (auto* texts : {&presented.sceneTexts, &presented.overlayTexts}) {
        for (auto& text : *texts) {
            text.fontSize *= visual.textScale;
            text.spacing *= visual.textScale;
        }
    }
    presented = withSettingsButton(std::move(presented), _window.getSize(), visual.textScale);
    presented.sceneTint = sceneTint(visual);
    Renderer(_resources).render(presented, stars);
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
