#pragma once

#include "GuiTypes.hpp"

#include <functional>
#include <optional>
#include <string>

namespace rtype::gui {

class HudPresenter
{
public:
    using TextMeasure = std::function<float(const std::string&, float)>;
    using TextureSize = std::function<std::optional<GuiVector2>(const std::string&)>;

    static void append(RenderScene& scene, const GuiRectangle& settingsButton, float textScale,
                       const TextMeasure& measure);
    static void appendPlayers(RenderScene& scene, GuiVector2 viewport, float textScale,
                              const TextMeasure& measure, const TextureSize& textureSize,
                              GuiPosition origin = {12.0F, 4.0F});
    static void appendGameOver(RenderScene& scene, GuiVector2 viewport, float textScale,
                               const TextMeasure& measure, const TextureSize& textureSize);
};

} // namespace rtype::gui