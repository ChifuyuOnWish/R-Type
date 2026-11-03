#pragma once

#include "GuiTypes.hpp"

namespace rtype::gui {

struct KeyboardState
{
    bool focused = true;
    std::array<bool, guiKeyCount> heldKeys{};
};

struct GamepadState
{
    bool connected = false;
    std::array<bool, guiGamepadButtonCount> heldButtons{};
    float leftX = 0.0F;
    float leftY = 0.0F;
};

[[nodiscard]] bool gamepadIsNeutral(const GamepadState& state, const GamepadSettings& settings);

class InputMapper
{
public:
    void setKeyBindings(const KeyBindings& bindings);
    void setGamepadSettings(const GamepadSettings& settings);
    [[nodiscard]] PlayerActions map(const KeyboardState& keyboard,
                                    const GamepadState& gamepad = {});
    void reset() noexcept;

private:
    KeyBindings _bindings;
    GamepadSettings _gamepad;
    bool _gamepadConnected = false;
    bool _gamepadReady = false;
    std::array<bool, playerActionCount> _heldActions{};
};

} // namespace rtype::gui
