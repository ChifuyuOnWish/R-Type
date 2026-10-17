#pragma once

#include "AudioSystem.hpp"
#include "IGui.hpp"
#include "InputMapper.hpp"
#include "ResourceManager.hpp"
#include "ScrollingBackground.hpp"
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
    Window _window;
    ResourceManager _resources;
    AudioSystem _audio;
    ScrollingBackground _background;
    InputMapper _input;
};

} // namespace rtype::gui
