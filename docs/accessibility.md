# Accessibility

## Physical and motor disabilities

The default controls use the arrow keys for movement and Space for fire.
Bindings can be changed through `IGui::setKeyBindings()`; the mapper also
supports letters, Enter, Control and Shift. Actions can share a key.
See [controls](controls.md) for the supported keys and an example.

Held actions are released when the window loses focus. Escape closes the
window and is reserved for window management.

Current limitations: there is no interactive rebinding menu, controller support,
toggle-to-fire option or one-handed preset. Changing bindings requires client
configuration through the API.

## Audio and visual disabilities

The demo identifies players with P1 to P4 labels as well as ship colors.
Its fire and enemy-removal events have visible text cues, including when audio
is muted, unavailable or an effect fails to load. Future gameplay events must
also provide the client with appropriate visual feedback.

Audio volume and mute are exposed through `IGui::setAudioSettings()`.
The demo accepts `--volume`, `--mute` and `--no-audio`.
See [audio](audio.md) for behavior and examples.

Status pages use light text on an opaque dark background. Status is expressed
by an explicit title and message rather than color or sound alone. Messages
wrap at word boundaries, and long individual words are split when necessary.
Layout uses the current window dimensions. At most 2048 message characters
are considered; text exceeding the available vertical space ends with an
ellipsis when a message line can be displayed.

Status pages are static: no flashing, pulsing, countdown or automatic dismissal.
The gameplay starfield can be disabled through
`scene.background.enabled = false`.

Current limitations: the built-in font has limited glyph coverage; there is no
screen-reader integration, text-size setting, colorblind palette selector or
scrollable error dialog. Very small windows may omit message text or the close
hint; check readability at 640 x 360 and above. Long diagnostic details belong
in client logs as well as the concise visible message.

## Mental and cognitive disabilities

Connecting, rejected, disconnected and error states have distinct English
headings. While such a status is supplied, the GUI hides gameplay sprites and
their overlays so stale positions do not look like a running game.

The client decides when a status changes. The GUI does not implement connection,
retry, timeout or recovery rules. It continues processing window events and
input; client coordination must decide whether gameplay actions are used.

Current limitations: there is no localization, tutorial or interactive reconnect
screen. Real multiplayer behavior still needs integration validation.

## Supply status feedback

```cpp
rtype::gui::RenderScene scene;
scene.feedback.status = rtype::gui::GuiStatus::Disconnected;
scene.feedback.message = "The server stopped responding.";
gui.render(scene);
```

Supported statuses are `None`, `Connecting`, `Rejected`, `Disconnected` and
`Error`. `None` leaves normal scene rendering active; it does not claim that
a network connection exists. An empty message uses a status-specific default.
Status rendering uses the built-in font and requires no custom visual assets.

Supply `None` with a current game scene when the client decides gameplay can
resume. Rendering a status does not clear the client's world or stop audio
already playing; those decisions belong to client coordination.

## Verification

Automated tests cover each status, default and custom messages, wrapping,
truncation, viewport bounds and layouts without gameplay entities.

Manual checks to perform:

- Run each status demo and verify readable headings, messages and the close hint.
- Resize the window and check that text stays inside it.
- Verify P1 to P4 labels and simultaneous movement/fire actions in the normal demo.
- Verify visible fire and enemy-removal cues with muted or unavailable audio.
- Confirm that a status hides the game view and Escape still closes the window.

```sh
./r-type_client --status connecting
./r-type_client --status rejected --message "The lobby is full."
./r-type_client --status disconnected --no-audio
./r-type_client --status error --message "Could not load the level."
```

These are local presentation checks, not live connection tests. Results for
real network events and platform-specific accessibility checks remain to be
recorded during integration.
