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
    std::string text;
    bool backspace = false;
    bool tab = false;
    bool shift = false;
    bool confirm = false;
    bool escape = false;
    bool settingsShortcut = false;
};

class InputReader
{
public:
    [[nodiscard]] InputFrame poll();
};

} // namespace rtype::gui
