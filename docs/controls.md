# Controls

## Defaults

| Action | Key |
|--------|-----|
| Move up | Up arrow |
| Move down | Down arrow |
| Move left | Left arrow |
| Move right | Right arrow |
| Fire | Space |

Escape closes the window and is reserved for window management.

## Action states

Call `IGui::pollActions()` once per frame before `render()`. Window events are
polled by raylib at the end of rendering and are read on the next iteration.

Each `PlayerActions` array is indexed by `PlayerAction`:

- `heldActions`: the action is currently held.
- `pressedActions`: the action became held since the previous poll.
- `releasedActions`: the action stopped being held since the previous poll.

Losing window focus clears held actions and reports releases for actions
previously held. While unfocused, keyboard presses are ignored. Held keys are
sampled again when focus returns.

The GUI reports controls only. Movement, firing rules and network transmission
are handled by the client and game systems.

## Change bindings

Bindings use `GuiKey`, not raylib key codes. Supported keys are the arrows,
Space, A to Z, Enter, and the left and right Control and Shift keys.
`GuiKey::None` disables an action.

Start from the defaults and change the entries you need:

```cpp
rtype::gui::KeyBindings bindings;
bindings.keys[static_cast<std::size_t>(rtype::gui::PlayerAction::Fire)] =
    rtype::gui::GuiKey::F;
gui.setKeyBindings(bindings);
```

`setKeyBindings()` is available through `IGui` before or after opening the
window. It replaces the complete binding set. Multiple actions may share one
key. Invalid key values throw without changing the current bindings.

Action transitions are evaluated on the next poll. Remapping away from a held
key reports a release if the replacement key is not held. Closing and reopening
the GUI clears input state and preserves bindings.

## Local demo

The temporary client demo displays held actions below the rendering scene and
prints press and release events to the terminal. It does not move ships or send
network messages.

Check the arrows and Space individually, then hold an arrow and press Space to
check simultaneous actions. Switch to another window while holding a key:
the next input poll must release the action.
