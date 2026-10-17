#pragma once

#include "GuiTypes.hpp"

namespace rtype::gui {

class ScrollingBackground
{
public:
    void update(const BackgroundConfig& config, float elapsedSeconds);
    [[nodiscard]] std::vector<GuiPosition> getStars(GuiVector2 viewport) const;

private:
    double _offset = 0.0;
};

} // namespace rtype::gui
