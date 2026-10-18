#pragma once

#include "GuiTypes.hpp"

namespace rtype::gui {

class StatusPresenter
{
public:
    [[nodiscard]] static RenderScene makeScene(const StatusFeedback& feedback, GuiVector2 viewport);
};

} // namespace rtype::gui
