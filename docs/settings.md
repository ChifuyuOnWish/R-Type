# Settings

## Open the menu

Press F1 or click the top-right Settings control from the connection screen,
lobby or game. Escape also opens Settings during gameplay. Escape or F1 closes
the menu, or cancels an active key capture first. Quit game is always available
in the bottom action bar.

Use a window of at least 640 x 360.

| Control | Effect |
|---------|--------|
| Tab / Shift+Tab | Next / previous category |
| Up / Down | Select an option or bottom action in the current category |
| Left / Right | Change a value |
| Enter or Space | Activate the selected row |
| Escape or F1 | Back, or cancel key capture |
| Mouse wheel | Move the selection |
| Click a row | Activate it |
| Click its left/right arrow | Decrease/increase a value |

Menu controls stay fixed when gameplay controls are rebound.

## Layout and categories

The menu uses the window width and height instead of a fixed 900-pixel column.
Category buttons on the left select Display, Audio, Controls and Accessibility.
Options are placed on notebook lines. Reset, Apply & save, Back and Quit game
remain visible at the bottom in every category, without scrolling to find them.
Text grows with the window and the configured text scale. On small windows, long
labels fit within their available area.

Display contains text size and Show FPS. Audio contains volume and mute. Controls
contains the keyboard bindings. Accessibility contains background motion, scene
tint and tint strength. Changing category preserves the draft.

An optional notebook background is loaded from `assets/ui/settings-background.png`.
Without it, the GUI draws light paper, horizontal rules and a margin. See the
[artwork guide](../assets/ui/README.md) for alignment and optional button images.

## Available settings

| Setting | Choices | Default |
|---------|---------|---------|
| Volume | 0 to 100%, steps of 5% | 50% |
| Mute | On / Off | Off |
| Background motion | On / Off | On |
| UI text size | 100%, 125%, 150% | 100% |
| Show FPS | On / Off | On |
| Scene tint | Off, Red, Green, Blue, Amber | Off |
| Tint opacity | 0 to 50%, steps of 5% | 15% |
| Keyboard bindings | Movement and fire | Arrows and Space |

The tint is a full-window colored rectangle blended over the game scene.
It covers the background, sprites and scene text. UI rectangles, overlay text,
Settings and connection status pages remain un-tinted. It requires no image
asset or shader. It is a visual preference; it is not a color-vision correction
or a simulation of a color-vision deficiency.

Text size scales menu text, connection status text and the scene/overlay text
submitted by the client. The GUI scales the font size and character spacing;
overlay positions remain in window pixels. When RenderScene::worldSize is set,
sprite destinations, origins and scene text are scaled to fit that world in
the window; overlay text and Settings keep their screen-space layout. The client must choose suitable
HUD positions and wrapping for larger text.

Disabling background motion freezes the starfield. It does not pause gameplay,
stop network updates or hide entities.

## Rebind keys

Activate a Key row, then press the new key. The activating press is not
captured as the replacement. If another action uses that key, the two bindings
are exchanged. Press Left on a Key row to remove its binding.
Escape or F1 cancels capture.

Supported keys are arrows, Space, A to Z, Enter, left/right Control and
left/right Shift. Escape and F1 are reserved for menu navigation.

## Apply and cancel

Changes stay in a draft until the fixed Apply & save button is selected. The menu
previews its text size and FPS visibility immediately. Apply updates the running GUI and writes preferences to
disk; the menu stays open and reports the result. Back, Escape or F1 discards
changes made since the last Apply.

Reset restores defaults in the draft. Apply & save keeps them. Quit game closes
the application; it does not save pending edits automatically.

Opening Settings releases gameplay actions. On startup, after API rebinding,
and after closing Settings, all sampled gameplay keys must be released before
input resumes. This avoids an activation key becoming an unintended shot.

The client must continue calling pollActions() and render() every frame while
Settings is open. The menu does not pause the client or a multiplayer session.

## Persistence

Settings are loaded when the GUI opens and saved only by Apply and save.

| Platform | Default file |
|----------|--------------|
| Linux with absolute XDG_CONFIG_HOME | $XDG_CONFIG_HOME/rtype/settings.conf |
| Linux otherwise | $HOME/.config/rtype/settings.conf |
| Windows | %APPDATA%/RType/settings.conf |

An explicit `GuiConfig::settingsPath` replaces the default location. The client
exposes it through `--settings path`. If no default directory is available,
settings still apply for the session and saving reports an error.

The version 1 text format stores volume, mute, background motion, text scale,
tint mode/opacity, `show-fps` and five keyboard bindings. Missing optional values use
defaults, and unknown entries are ignored. Unsupported versions, duplicate
entries, invalid values or files larger than 16 KiB are rejected without
partially applying settings.

Saving writes a temporary sibling file before replacing the previous file.
A write error leaves the existing destination in place where replacement
failed; details are logged to the terminal. If saving fails, the new values
still apply for the current session. Invalid files are not overwritten until
the user explicitly selects Apply and save.

Explicit API calls to setKeyBindings() or setAudioSettings() before opening take
precedence over the corresponding saved preferences. API updates are not
automatically written to disk.

## Local checks

```sh
cmake --preset release
cmake --build --preset release
ctest --preset release
./r-type_client
```

To use a separate test file on Linux:

```sh
./r-type_client --settings /tmp/rtype-settings-check.conf
```

Without --connect, the client shows the connection form with Settings available.
To check movement against the authoritative server, use two terminals:

```sh
./r-type_server 4242
```

```sh
./r-type_client --connect 127.0.0.1:4242 --name Player
```

Only server entities are rendered. No local fixture ships, projectiles or
scheduled enemy removals are created. The client opens at 1920 x 1080. Its
1920 x 1080 world fits the current window with uniform scaling and centered
placement, preserving its aspect ratio.

1. Open and close Settings with F1, Escape and the mouse.
2. Rebind an action, apply, close Settings and verify movement in the connected game.
3. Assign an already used key and check that the bindings are exchanged.
4. Hold Fire, open the menu and verify that gameplay actions are released.
   After closing, release the key before testing Fire again.
5. While connected, fire and check server-confirmed shot sounds. Change volume
   and mute, apply, restart and verify the saved values. Mute and --no-audio must
   suppress sound without preventing movement or rendering.
6. Disable background motion and check that the stars stop scrolling.
7. Apply each tint and change its opacity. The scene changes color while
   Settings and overlay text remain readable.
8. Set text size to 150% and resize. Check labels, footer messages and HUD text.
9. Apply settings, restart and check that they are restored.
10. Change a setting then Back. Reopen and check that the change was discarded.
11. Connect to an unavailable port, wait for failure and verify Settings remains reachable.
12. Switch categories and verify all four bottom actions remain available.
13. Disable Show FPS, save, restart and verify the preference remains disabled.
14. Enable Show FPS and verify the counter is at the top left in menus and gameplay.
15. Quit through the bottom action in Settings.

Automated tests cover draft edits, binding swaps, persistence/error paths,
menu transitions, categories, fixed footer actions, FPS visibility and persistence,
text scaling and tint drawing order using fake native
boundaries. These tests do not validate actual window rendering or audio
hardware. Record manual results on supported platforms before merging.

## FPS counter

The GUI calculates the average frame rate over approximately half a second.
When enabled, the counter is positioned at x=12, y=8 in window pixels, independently
of world scaling. It remains at the top in gameplay, the connection screen, lobby
and Settings. The option controls display only; it does not change the target FPS.
