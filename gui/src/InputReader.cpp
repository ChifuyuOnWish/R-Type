#include "InputReader.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <raylib.h>
#include <string>

namespace rtype::gui {

namespace {

constexpr std::array buttonCodes{
    GAMEPAD_BUTTON_UNKNOWN,          GAMEPAD_BUTTON_LEFT_FACE_UP,    GAMEPAD_BUTTON_LEFT_FACE_DOWN,
    GAMEPAD_BUTTON_LEFT_FACE_LEFT,   GAMEPAD_BUTTON_LEFT_FACE_RIGHT, GAMEPAD_BUTTON_RIGHT_FACE_DOWN,
    GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, GAMEPAD_BUTTON_RIGHT_FACE_LEFT, GAMEPAD_BUTTON_RIGHT_FACE_UP,
    GAMEPAD_BUTTON_LEFT_TRIGGER_1,   GAMEPAD_BUTTON_RIGHT_TRIGGER_1, GAMEPAD_BUTTON_LEFT_TRIGGER_2,
    GAMEPAD_BUTTON_RIGHT_TRIGGER_2,  GAMEPAD_BUTTON_LEFT_THUMB,      GAMEPAD_BUTTON_RIGHT_THUMB};
static_assert(buttonCodes.size() == guiGamepadButtonCount);

int repeat(int direction, int& previous, float& timer, float dt)
{
    if (direction != previous) {
        previous = direction;
        timer = 0.35F;
        return direction;
    }
    if (direction == 0)
        return 0;
    timer -= std::isfinite(dt) ? std::clamp(dt, 0.0F, 0.1F) : 0.0F;
    if (timer > 0.0F)
        return 0;
    timer = 0.1F;
    return direction;
}

} // namespace

void InputReader::loadMappings(const std::filesystem::path& path)
{
    std::error_code error;
    if (!std::filesystem::exists(path, error))
        return;
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    const auto size = file.tellg();
    if (!file || size <= 0 || size > 4 * 1024 * 1024) {
        TraceLog(LOG_WARNING, "Gamepad mapping database is unreadable or too large");
        return;
    }
    file.seekg(0);
    std::string mappings(static_cast<std::size_t>(size), '\0');
    if (!file.read(mappings.data(), static_cast<std::streamsize>(mappings.size())) ||
        SetGamepadMappings(mappings.c_str()) == 0)
        TraceLog(LOG_WARNING, "Could not register the gamepad mapping database");
}

void InputReader::reset() noexcept
{
    _vertical = 0;
    _horizontal = 0;
    _verticalRepeat = 0.0F;
    _horizontalRepeat = 0.0F;
    _device = -1;
    _menuReady = false;
    _connected = false;
}

InputFrame InputReader::poll(const GamepadSettings& settings)

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
    if (_device != settings.device) {
        reset();
        _device = settings.device;
    }
    frame.gamepadDetected = settings.enabled && IsGamepadAvailable(settings.device);
    frame.gamepad.connected = frame.gamepadDetected && glfwJoystickIsGamepad(settings.device);
    if (frame.gamepadDetected) {
        if (const char* name = GetGamepadName(settings.device))
            frame.gamepadName = name;
        if (!frame.gamepad.connected) {
            const char* guid = glfwGetJoystickGUID(settings.device);
            if (guid != nullptr && _lastUnmappedGuid != guid) {
                _lastUnmappedGuid = guid;
                TraceLog(
                    LOG_WARNING,
                    "Unmapped controller GUID: %s. No matching layout in the bundled databases.",
                    guid);
            }
        }
    }
    if (!frame.keyboard.focused) {
        reset();
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

    if (frame.gamepad.connected) {
        for (std::size_t index = 1; index < buttonCodes.size(); ++index) {
            frame.gamepad.heldButtons[index] =
                IsGamepadButtonDown(settings.device, buttonCodes[index]);
        }
        frame.gamepad.leftX = GetGamepadAxisMovement(settings.device, GAMEPAD_AXIS_LEFT_X);
        frame.gamepad.leftY = GetGamepadAxisMovement(settings.device, GAMEPAD_AXIS_LEFT_Y);
        if (!_connected)
            _menuReady = false;
        _connected = true;
        auto navigationSettings = settings;
        navigationSettings.leftStick = true;
        if (gamepadIsNeutral(frame.gamepad, navigationSettings) &&
            !IsGamepadButtonDown(settings.device, GAMEPAD_BUTTON_MIDDLE_RIGHT))
            _menuReady = true;
        if (!_menuReady)
            return frame;
        for (std::size_t index = 1; index < buttonCodes.size(); ++index)
            if (!frame.menu.gamepadButton &&
                IsGamepadButtonPressed(settings.device, buttonCodes[index]))
                frame.menu.gamepadButton = static_cast<GuiGamepadButton>(index);
        const auto down = [&](GuiGamepadButton button) {
            return frame.gamepad.heldButtons[static_cast<std::size_t>(button)];
        };
        const float threshold = std::max(0.5F, settings.deadZone);
        const bool up = down(GuiGamepadButton::DpadUp) || frame.gamepad.leftY < -threshold;
        const bool below = down(GuiGamepadButton::DpadDown) || frame.gamepad.leftY > threshold;
        const bool left = down(GuiGamepadButton::DpadLeft) || frame.gamepad.leftX < -threshold;
        const bool right = down(GuiGamepadButton::DpadRight) || frame.gamepad.leftX > threshold;
        const int vertical = repeat(static_cast<int>(below) - static_cast<int>(up), _vertical,
                                    _verticalRepeat, GetFrameTime());
        const int horizontal = repeat(static_cast<int>(right) - static_cast<int>(left), _horizontal,
                                      _horizontalRepeat, GetFrameTime());
        if (frame.menu.vertical == 0)
            frame.menu.vertical = vertical;
        if (frame.menu.horizontal == 0)
            frame.menu.horizontal = horizontal;
        frame.menu.activate |=
            IsGamepadButtonPressed(settings.device, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        frame.menu.back |= IsGamepadButtonPressed(settings.device, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
        const bool start = IsGamepadButtonPressed(settings.device, GAMEPAD_BUTTON_MIDDLE_RIGHT);
        frame.toggleSettings |= start;
        frame.menu.cancelCapture |= start;
    } else {
        _connected = false;
        _menuReady = false;
        _vertical = _horizontal = 0;
    }
    return frame;
}

} // namespace rtype::gui
