#pragma once

#include "AudioSystem.hpp"
#include "FrontendMenu.hpp"
#include "IGui.hpp"
#include "InputMapper.hpp"
#include "InputReader.hpp"
#include "ResourceManager.hpp"
#include "ScrollingBackground.hpp"
#include "SettingsMenu.hpp"
#include "Window.hpp"

namespace rtype::gui {

class Gui final : public IGui
{
public:
    explicit Gui(std::filesystem::path assetRoot = "assets");
    ~Gui() override;

    void open(const GuiConfig& config) override;
    void close() noexcept override;

    [[nodiscard]] bool shouldClose() const override;
    void setKeyBindings(const KeyBindings& bindings) override;
    [[nodiscard]] PlayerActions pollActions() override;
    [[nodiscard]] bool isSettingsOpen() const noexcept override;
    [[nodiscard]] const GuiPreferences& getPreferences() const noexcept override;
    void render(const RenderScene& scene) override;
    void setAudioSettings(const AudioSettings& settings) override;
    [[nodiscard]] bool isAudioAvailable() const noexcept override;
    void playAudio(std::span<const SoundEvent> events) override;
    [[nodiscard]] float getFrameTime() const override;

    [[nodiscard]] AudioSystem& audio() noexcept;
    [[nodiscard]] const AudioSystem& audio() const noexcept;

    [[nodiscard]] ResourceManager& resources() noexcept;
    [[nodiscard]] const ResourceManager& resources() const noexcept;

private:
    void applyPreferences(const GuiPreferences& preferences);
    Window _window;
    ResourceManager _resources;
    AudioSystem _audio;
    ScrollingBackground _background;
    InputMapper _input;
    InputReader _reader;
    SettingsMenu _menu;
    FrontendMenu _frontend;
    GuiPreferences _preferences;
    std::filesystem::path _settingsPath;
    std::string _settingsNotice;
    std::string _uiFont;
    bool _keyOverride = false;
    bool _audioOverride = false;
    bool _quitRequested = false;
    bool _waitForNeutral = true;
    float _fpsElapsed = 0.0F;
    unsigned _fpsFrames = 0;
    unsigned _displayedFps = 0;
};

} // namespace rtype::gui
