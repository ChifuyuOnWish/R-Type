# Accessibility

## Physical and motor access

Players can rebind movement and fire through a keyboard/mouse Settings menu.
Menu navigation remains fixed after rebinding. Gameplay input is released on
focus loss and while Settings is open. Returning from Settings waits for keys
to be released before gameplay resumes.

See [controls](controls.md) and [Settings](settings.md).

Limitations: no controller support, toggle-to-fire mode or one-handed preset.
Holding Fire is reported continuously; client/game systems own firing behavior.

## Audio and visual access

Settings provides volume, mute and text sizes of 100%, 125% and 150%.
Audio failures leave input and rendering operational. Gameplay sound events
are not connected in the current client; visual equivalents must accompany
future sound feedback.

Player labels P1 to P4 supplement the colors of server-controlled ships.
Connection statuses use explicit headings and messages instead of color or
sound alone. They use light text on a dark background, wrap messages and
truncate when the available vertical space is exhausted.

An optional scene tint offers red, green, blue or amber at adjustable opacity.
This is a visual preference, not a validated color-vision correction.
Essential information must remain identifiable without color.

Text-size settings apply to Settings, status pages and client-supplied scene
and overlay text. Font size and spacing change; client-supplied positions do
not. Client HUD layout must accommodate larger text.

Limitations: no screen-reader integration or localization. The built-in font
has limited glyph coverage. Real window readability still requires manual
checks, including 640 x 360 and large text.

## Mental and cognitive access

Settings uses labeled values, explicit Apply and save, Back and Restore
defaults actions. Unapplied changes are discarded on Back, and saved settings
are restored on later launches.

Background motion can be disabled independently of game simulation.
Status pages are static and do not flash or dismiss themselves. When a status
is supplied, it hides the game view to avoid showing stale positions.

Limitations: no tutorial, simplified difficulty setting or interactive reconnect
screen. The GUI does not decide connection, retry, timeout or recovery rules.

## Supply status feedback

```cpp
rtype::gui::RenderScene scene;
scene.feedback.status = rtype::gui::GuiStatus::Disconnected;
scene.feedback.message = "The server stopped responding.";
gui.render(scene);
```

Supported statuses are None, Connecting, Rejected, Disconnected and Error.
An empty message selects a status-specific default. Status rendering uses the
built-in font without custom assets. Supply None with a current scene when
gameplay may resume.

Continue calling pollActions() on status pages so Settings remains usable.
Rendering a status does not clear the client world or stop existing audio.

## Verification

See the [Settings checklist](settings.md#local-checks). Automated checks cover
settings, keyboard mapping, persistence, status layouts and tint drawing order.
Physical rendering, audio and multiplayer integration require manual checks.
No accessibility certification is claimed.

