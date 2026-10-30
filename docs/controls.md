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

## Gamepad controls

Gamepad support is enabled by default. In Settings, Controller selects one of
four device slots (shown as 1 to 4). The active device and connection state are
shown above the rows. Apply and save after changing the device before rebinding
its buttons. Keyboard and mouse remain available.

| Action | Default controller input |
|--------|--------------------------|
| Movement | Left stick or D-pad |
| Fire | South face button (Xbox A / PlayStation Cross) |
| Open/close Settings | Start / Menu |
| Menu selection | D-pad or left stick |
| Menu value | Left / Right |
| Activate a row | South face button |
| Back | East face button (Xbox B / PlayStation Circle) |
| Cancel a binding capture | Start, Escape or F1 |

Menu buttons are fixed and independent of gameplay bindings. The selected
controller contributes to the same PlayerActions arrays as the keyboard; no
new network input format is introduced. A held action remains active if either
input source still holds it.

The movement dead zone defaults to 20% and can be adjusted from 5% to 80%.
The left stick can be disabled for gameplay while remaining usable in menus.
The right stick is not used. Menu stick navigation has a minimum threshold of
50% and repeats after a short delay, preventing one deflection from skipping
many rows immediately.

A newly connected or refocused controller must return to neutral before its
buttons or axes can trigger gameplay. Disconnecting it releases its actions;
keyboard actions can continue. Opening Settings releases gameplay actions, and
closing it waits for keyboard keys and relevant pad inputs to return to neutral.

The GUI loads the generic mapping database at
the single bundled database at `assets/input/gamecontrollerdb.txt`. Missing mappings are reported
in Settings and logged with the GUID. This distinguishes an unmapped device
from an unplugged controller. See `assets/input/README.md` for provenance and
updates. Physical device support still requires a manual check.

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


