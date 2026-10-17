#include "InputMapper.hpp"

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

PlayerActions InputMapper::map(const KeyboardState& keyboard)
{
    PlayerActions actions;
    for (std::size_t index = 0; index < playerActionCount; ++index) {
        const auto key = static_cast<std::size_t>(_bindings.keys[index]);
        const bool active = keyboard.focused && _bindings.keys[index] != GuiKey::None;
        const bool held = active && keyboard.heldKeys[key];
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
}

} // namespace rtype::gui
