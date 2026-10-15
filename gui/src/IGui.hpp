#pragma once

#include "GuiTypes.hpp"

#include <span>

namespace rtype::gui {

class IGui
{
public:
    virtual ~IGui() = default;

    // Open once before use. Invalid configuration or initialization failure throws.
    virtual void open(const GuiConfig& config) = 0;

    // Safe to call repeatedly; implementations also release resources on destruction.
    virtual void close() noexcept = 0;

    // Return true when closed or when the user requests closure.
    [[nodiscard]] virtual bool shouldClose() const = 0;

    // Call once per frame before rendering to capture action transitions.
    [[nodiscard]] virtual PlayerActions pollActions() = 0;

    // Borrow the scene for this call only; do not retain references to its contents.
    virtual void render(const RenderScene& scene) = 0;

    // Consume each batch once; do not retain the span.
    virtual void playAudio(std::span<const SoundEvent> events) = 0;

    // Seconds elapsed during the previous completed frame.
    [[nodiscard]] virtual float getFrameTime() const = 0;
};

} // namespace rtype::gui
