#pragma once

#include "IGui.hpp"
#include "ResourceManager.hpp"
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
    [[nodiscard]] PlayerActions pollActions() override;
    void render(const RenderScene& scene) override;
    void playAudio(std::span<const SoundEvent> events) override;
    [[nodiscard]] float getFrameTime() const override;

    [[nodiscard]] ResourceManager& resources() noexcept;
    [[nodiscard]] const ResourceManager& resources() const noexcept;

private:
    Window _window;
    ResourceManager _resources;
};

} // namespace rtype::gui
