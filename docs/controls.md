# Controls

## Defaults

| Action | Key |
|--------|-----|
| Move up | Up arrow |
| Move down | Down arrow |
| Move left | Left arrow |
| Move right | Right arrow |
| Fire | Space |
| Open/close Settings | F1 or Escape |

The Settings button can also be clicked. Escape no longer closes the window.
Use the close button or Quit application in Settings to exit.

Change gameplay bindings through [Settings](settings.md). Supported keys are
arrows, Space, A to Z, Enter, and left/right Control and Shift.

## Action states

Call `IGui::pollActions()` once per frame before rendering. Raylib polls native
window events at the end of rendering and exposes them on the next iteration.

Each PlayerActions array is indexed by PlayerAction:

- heldActions: the action is currently held.
- pressedActions: the action became held since the previous poll.
- releasedActions: the action stopped being held since the previous poll.

Focus loss releases actions; unfocused input is ignored. Held keys are sampled
again when focus returns. Settings releases gameplay actions while it is open.
On startup, after API rebinding and after closing Settings, sampled keys must
return to neutral before gameplay input resumes.

The GUI reports actions. Client/game systems own movement, firing behavior
and network transmission.

## Programmatic bindings

Bindings use GuiKey without exposing native key codes.

```cpp
rtype::gui::KeyBindings bindings;
bindings.keys[static_cast<std::size_t>(rtype::gui::PlayerAction::Fire)] =
    rtype::gui::GuiKey::F;
gui.setKeyBindings(bindings);
```

The setter replaces the complete binding set. GuiKey::None disables a binding.
Multiple actions may share a key through the API; interactive rebinding swaps
conflicting bindings. Invalid key values throw without changing the bindings.

See [Settings](settings.md) for saving, restoring and local verification.

