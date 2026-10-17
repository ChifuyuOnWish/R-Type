#include "InputMapper.hpp"

#include <algorithm>
#include <stdexcept>

namespace {

using namespace rtype::gui;

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::size_t index(PlayerAction action)
{
    return static_cast<std::size_t>(action);
}

std::size_t index(GuiKey key)
{
    return static_cast<std::size_t>(key);
}

bool any(const std::array<bool, playerActionCount>& actions)
{
    return std::any_of(actions.begin(), actions.end(), [](bool value) { return value; });
}

void checkDefaultBindings()
{
    constexpr std::array defaultKeys{GuiKey::Up, GuiKey::Down, GuiKey::Left, GuiKey::Right,
                                     GuiKey::Space};
    for (std::size_t action = 0; action < playerActionCount; ++action) {
        InputMapper mapper;
        KeyboardState keyboard;
        const auto key = index(defaultKeys[action]);
        keyboard.heldKeys[key] = true;
        auto result = mapper.map(keyboard);
        require(result.heldActions[action] && result.pressedActions[action],
                "Default binding must produce the expected action");
        require(std::count(result.heldActions.begin(), result.heldActions.end(), true) == 1,
                "A default key must trigger exactly one action");

        result = mapper.map(keyboard);
        require(result.heldActions[action] && !any(result.pressedActions) &&
                    !any(result.releasedActions),
                "Holding a key must not repeat transitions");

        keyboard.heldKeys[key] = false;
        result = mapper.map(keyboard);
        require(!any(result.heldActions) && result.releasedActions[action],
                "Releasing a key must release its action");
        result = mapper.map({});
        require(!any(result.heldActions) && !any(result.pressedActions) &&
                    !any(result.releasedActions),
                "Release transitions must not repeat on the next frame");
    }
}

void checkFocus()
{
    InputMapper mapper;
    KeyboardState keyboard;
    keyboard.heldKeys[index(GuiKey::Up)] = true;
    keyboard.heldKeys[index(GuiKey::Space)] = true;
    auto result = mapper.map(keyboard);
    require(result.heldActions[index(PlayerAction::MoveUp)] &&
                result.heldActions[index(PlayerAction::Fire)],
            "Movement and fire must work together");

    keyboard.focused = false;
    result = mapper.map(keyboard);
    require(!any(result.heldActions) && !any(result.pressedActions),
            "Unfocused input must not hold or press actions");
    require(result.releasedActions[index(PlayerAction::MoveUp)] &&
                result.releasedActions[index(PlayerAction::Fire)],
            "Focus loss must release previously held actions");
    require(!any(mapper.map(keyboard).releasedActions), "Focus loss releases must occur once");

    keyboard.focused = true;
    result = mapper.map(keyboard);
    require(result.heldActions[index(PlayerAction::Fire)] &&
                result.pressedActions[index(PlayerAction::Fire)],
            "Regaining focus must sample current held keys");

    mapper.reset();
    result = mapper.map({});
    require(!any(result.releasedActions), "Reset must discard the previous held state");
}

void checkBindings()
{
    InputMapper mapper;
    KeyboardState keyboard;
    keyboard.heldKeys[index(GuiKey::Space)] = true;
    (void)mapper.map(keyboard);

    KeyBindings bindings;
    bindings.keys[index(PlayerAction::Fire)] = GuiKey::F;
    mapper.setKeyBindings(bindings);
    auto result = mapper.map(keyboard);
    require(!result.heldActions[index(PlayerAction::Fire)] &&
                result.releasedActions[index(PlayerAction::Fire)],
            "Remapping away from a held key must release the action");

    keyboard.heldKeys[index(GuiKey::F)] = true;
    result = mapper.map(keyboard);
    require(result.heldActions[index(PlayerAction::Fire)] &&
                result.pressedActions[index(PlayerAction::Fire)],
            "The replacement key must trigger the action");

    KeyBindings invalid = bindings;
    invalid.keys[index(PlayerAction::Fire)] = GuiKey::Count;
    bool rejected = false;
    try {
        mapper.setKeyBindings(invalid);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && mapper.map(keyboard).heldActions[index(PlayerAction::Fire)],
            "Invalid bindings must be rejected without changing existing bindings");

    bindings.keys[index(PlayerAction::Fire)] = GuiKey::None;
    mapper.setKeyBindings(bindings);
    keyboard.heldKeys[index(GuiKey::None)] = true;
    result = mapper.map(keyboard);
    require(!result.heldActions[index(PlayerAction::Fire)] &&
                result.releasedActions[index(PlayerAction::Fire)],
            "None must disable an action and release its previous state");

    bindings.keys[index(PlayerAction::Fire)] = GuiKey::Up;
    mapper.setKeyBindings(bindings);
    keyboard = {};
    keyboard.heldKeys[index(GuiKey::Up)] = true;
    result = mapper.map(keyboard);
    require(result.heldActions[index(PlayerAction::MoveUp)] &&
                result.heldActions[index(PlayerAction::Fire)],
            "One key may be bound to multiple actions");
}

} // namespace

int main()
{
    checkDefaultBindings();
    checkFocus();
    checkBindings();
}
