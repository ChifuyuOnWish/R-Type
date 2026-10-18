#pragma once

#include "InputMapper.hpp"
#include "SettingsMenu.hpp"

namespace rtype::gui {

struct InputFrame
{
    KeyboardState keyboard;
    MenuInput menu;
    bool toggleSettings = false;
    bool mouseClick = false;
    GuiPosition mouse;
    float wheel = 0.0F;
};

class InputReader
{
public:
    [[nodiscard]] InputFrame poll();
};

} // namespace rtype::gui
