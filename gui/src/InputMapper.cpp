#include "InputMapper.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace rtype::gui {

void InputMapper::setKeyBindings(const KeyBindings& bindings)
{
    for (const auto key : bindings.keys) {
        if (static_cast<std::size_t>(key) >= guiKeyCount)
            throw std::invalid_argument("Invalid GUI key binding");
    }
    _bindings = bindings;
}

bool gamepadIsNeutral(const GamepadState& state, const GamepadSettings& settings)
{
    const bool buttonsReleased = std::none_of(
        state.heldButtons.begin() + 1, state.heldButtons.end(), [](bool held) { return held; });
    return !state.connected ||
           (buttonsReleased &&
            (!settings.leftStick || (std::isfinite(state.leftX) && std::isfinite(state.leftY) &&
                                     std::abs(state.leftX) <= settings.deadZone &&
                                     std::abs(state.leftY) <= settings.deadZone)));
}

void InputMapper::setGamepadSettings(const GamepadSettings& settings)
{
    if (settings.device < 0 || settings.device > 3 || !std::isfinite(settings.deadZone) ||
        settings.deadZone < 0.05F || settings.deadZone > 0.8F)
        throw std::invalid_argument("Invalid gamepad device or dead zone");
    for (const auto button : settings.buttons) {
        if (static_cast<std::size_t>(button) >= guiGamepadButtonCount)
            throw std::invalid_argument("Invalid gamepad button binding");
    }
    _gamepad = settings;
    _gamepadConnected = false;
    _gamepadReady = false;
}

PlayerActions InputMapper::map(const KeyboardState& keyboard, const GamepadState& gamepad)
{
    const bool connected = keyboard.focused && _gamepad.enabled && gamepad.connected;
    if (!connected || !_gamepadConnected)
        _gamepadReady = false;
    if (connected && gamepadIsNeutral(gamepad, _gamepad))
        _gamepadReady = true;
    _gamepadConnected = connected;

    std::array<bool, playerActionCount> padActions{};
    if (connected && _gamepadReady) {
        for (std::size_t index = 0; index < playerActionCount; ++index) {
            const auto button = _gamepad.buttons[index];
            padActions[index] = button != GuiGamepadButton::None &&
                                gamepad.heldButtons[static_cast<std::size_t>(button)];
        }
        if (_gamepad.leftStick && std::isfinite(gamepad.leftX) && std::isfinite(gamepad.leftY)) {
            padActions[static_cast<std::size_t>(PlayerAction::MoveUp)] |=
                gamepad.leftY < -_gamepad.deadZone;
            padActions[static_cast<std::size_t>(PlayerAction::MoveDown)] |=
                gamepad.leftY > _gamepad.deadZone;
            padActions[static_cast<std::size_t>(PlayerAction::MoveLeft)] |=
                gamepad.leftX < -_gamepad.deadZone;
            padActions[static_cast<std::size_t>(PlayerAction::MoveRight)] |=
                gamepad.leftX > _gamepad.deadZone;
        }
    }
    PlayerActions actions;
    for (std::size_t index = 0; index < playerActionCount; ++index) {
        const auto key = static_cast<std::size_t>(_bindings.keys[index]);
        const bool fromKey =
            keyboard.focused && _bindings.keys[index] != GuiKey::None && keyboard.heldKeys[key];
        const bool held = fromKey || padActions[index];
        actions.heldActions[index] = held;
        actions.pressedActions[index] = held && !_heldActions[index];
        actions.releasedActions[index] = _heldActions[index] && !held;
    }
    _heldActions = actions.heldActions;
    return actions;
}

void InputMapper::reset() noexcept
{
    _heldActions = {};
    _gamepadConnected = false;
    _gamepadReady = false;
}

} // namespace rtype::gui
