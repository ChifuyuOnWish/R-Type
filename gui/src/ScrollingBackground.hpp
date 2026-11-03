#pragma once

#include "GuiTypes.hpp"

namespace rtype::gui {

class ScrollingBackground
{
public:
    void update(const BackgroundConfig& config, float elapsedSeconds);
    [[nodiscard]] std::vector<GuiPosition> getStars(GuiVector2 viewport) const;
    [[nodiscard]] std::vector<SpriteDraw> getTiles(GuiVector2 viewport, GuiVector2 textureSize,
                                                   const std::string& textureId) const;

private:
    double _offset = 0.0;
    double _paperDistance = 0.0;
};

} // namespace rtype::gui
