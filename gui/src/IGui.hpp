#pragma once

#include "GuiTypes.hpp"

#include <span>

namespace rtype::gui {

class IGui
{
public:
    virtual ~IGui() = default;

    virtual void open(const GuiConfig& config) = 0;

    virtual void close() noexcept = 0;

    [[nodiscard]] virtual bool shouldClose() const = 0;

    virtual void setKeyBindings(const KeyBindings& bindings) = 0;

    [[nodiscard]] virtual PlayerActions pollActions() = 0;

    virtual void render(const RenderScene& scene) = 0;

    virtual void playAudio(std::span<const SoundEvent> events) = 0;

    [[nodiscard]] virtual float getFrameTime() const = 0;
};

} // namespace rtype::gui
