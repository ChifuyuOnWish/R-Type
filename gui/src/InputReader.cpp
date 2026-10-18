#include "InputReader.hpp"

#include <raylib.h>

namespace rtype::gui {

InputFrame InputReader::poll()
{
    constexpr std::array keyCodes{
        KEY_NULL,
        KEY_UP,
        KEY_DOWN,
        KEY_LEFT,
        KEY_RIGHT,
        KEY_SPACE,
        KEY_A,
        KEY_B,
        KEY_C,
        KEY_D,
        KEY_E,
        KEY_F,
        KEY_G,
        KEY_H,
        KEY_I,
        KEY_J,
        KEY_K,
        KEY_L,
        KEY_M,
        KEY_N,
        KEY_O,
        KEY_P,
        KEY_Q,
        KEY_R,
        KEY_S,
        KEY_T,
        KEY_U,
        KEY_V,
        KEY_W,
        KEY_X,
        KEY_Y,
        KEY_Z,
        KEY_ENTER,
        KEY_LEFT_CONTROL,
        KEY_RIGHT_CONTROL,
        KEY_LEFT_SHIFT,
        KEY_RIGHT_SHIFT,
    };
    InputFrame frame;
    frame.keyboard.focused = IsWindowFocused();
    if (!frame.keyboard.focused) {
        return frame;
    }
    for (std::size_t index = 1; index < keyCodes.size(); ++index) {
        frame.keyboard.heldKeys[index] = IsKeyDown(keyCodes[index]);
        if (!frame.menu.key && IsKeyPressed(keyCodes[index]))
            frame.menu.key = static_cast<GuiKey>(index);
    }
    const auto pressed = [](int key) { return IsKeyPressed(key) || IsKeyPressedRepeat(key); };
    frame.menu.vertical = pressed(KEY_UP) ? -1 : pressed(KEY_DOWN) ? 1 : 0;
    frame.menu.horizontal = pressed(KEY_LEFT) ? -1 : pressed(KEY_RIGHT) ? 1 : 0;
    frame.menu.activate = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    frame.menu.back = IsKeyPressed(KEY_ESCAPE);
    frame.toggleSettings = IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_F1);
    frame.menu.cancelCapture = frame.toggleSettings;
    const auto mouse = GetMousePosition();
    frame.mouse = {mouse.x, mouse.y};
    frame.mouseClick = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    frame.wheel = GetMouseWheelMove();

    return frame;
}

} // namespace rtype::gui
