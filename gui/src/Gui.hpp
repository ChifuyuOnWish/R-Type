#pragma once

#include "IGui.hpp"
#include "Window.hpp"

namespace rtype::gui {

class Gui final : public IGui
{
public:
    void open(const GuiConfig& config) override;
    void close() noexcept override;

    [[nodiscard]] bool shouldClose() const override;
    [[nodiscard]] PlayerActions pollActions() override;
    void render(const RenderScene& scene) override;
    void playAudio(std::span<const SoundEvent> events) override;
    [[nodiscard]] float getFrameTime() const override;

private:
    Window _window;
};

} // namespace rtype::gui
