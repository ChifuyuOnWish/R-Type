# Add a GUI resource

## Choose a path and an ID

Place the file in the appropriate `assets/` directory. Existing categories
include `player/`, `enemy/`, `boss/`, `projectiles/`, `effects/` and `audio/`.
Record the source and license in `assets/README.md` when adding new content.

Paths below are relative to the asset root beside the executable. Filename
capitalization must match on Linux and Windows. A resource ID is a separate
application name: moving a file requires updating its load path, while scene
data may keep the same ID.

Open `Gui` before registering resources. Register once at startup, not on every
frame. Texture, font and sound IDs have separate namespaces.

## Add a texture and a sprite

The following uses an existing asset so the coordinates are reproducible:

```cpp
gui.resources().loadTexture("players", "player/ship-variants.gif");
const auto size = gui.resources().getTextureSize("players");
```

Inspect the sheet and choose a source region within its dimensions. Then add
drawing data to the scene:

```cpp
rtype::gui::SpriteDraw ship;
ship.entityId = {1, 0};
ship.textureId = "players";
ship.sourceRect = {{67.0F, 2.0F}, 32.0F, 15.0F};
ship.destinationRect = {{100.0F, 100.0F}, 96.0F, 45.0F};
ship.layer = 10;
scene.sprites.push_back(ship);
```

Destination position is the top-left corner when origin is zero. When positions
come from the server, convert its center coordinates before submitting the
sprite. Higher layers draw later; equal layers preserve submission order.

Repeat the sprite in every scene while it should remain visible. To animate a
sheet, select different `sourceRect` regions over time; loading a GIF does not
start an animation automatically. Texture regions must have positive dimensions
and stay inside the sheet. The renderer rejects invalid or unknown resources
before starting the frame.

## Add a custom font

Place a supported font file, for example at `assets/fonts/hud.ttf`, and register
it after opening the GUI. This example path is a file to add, not a bundled font.

```cpp
gui.resources().loadFont("hud", "fonts/hud.ttf", 32);

rtype::gui::TextDraw title;
title.fontId = "hud";
title.text = "R-Type";
title.position = {20.0F, 20.0F};
title.fontSize = 24.0F;
scene.overlayTexts.push_back(title);
```

An empty font ID uses raylib's borrowed default font. A custom load that falls
back to that default is treated as a failure. The current font loader does not
provide a custom Unicode glyph list, so verify the characters actually needed.

## Add a sound effect

Load the sound when audio is available, and retain whether registration worked:

```cpp
bool shotLoaded = false;
if (gui.isAudioAvailable()) {
    shotLoaded = gui.audio().loadSound("shot", "audio/demo-shot.wav");
    if (!shotLoaded)
        std::cerr << gui.audio().getLastError() << '\n';
}
```

When a validated gameplay event arrives, queue its sound once:

```cpp
if (shotLoaded)
    soundEvents.push_back({"shot"});
```

Call `gui.playAudio(soundEvents)` once per client frame, then discard the events
before collecting the next frame. Do not replay a sound just because its entity
remains visible. The current client skips new gameplay sounds while Settings or
a connection status page is shown.

Use the existing `GameEvent` handler in `client/src/main.cpp` when changing sound
mapping. For a new effect, add its file, choose an ID, register it, and select
that ID for the corresponding event. The two current WAV files are generated
placeholder tones. See [audio](../audio.md) for volume, mute and voice limits.

## Handle failures and verify

Texture and font registration throw exceptions containing the resource ID and
resolved path. Repeating the same ID, normalized path and font size reuses the
resource; conflicting registrations throw. Sound registration returns false
with an error string for conflicts and loading failures. Failed loads are not
cached.

Check the resource on Linux and Windows, at small and large window sizes and
with 150% text size. For sounds, also check mute, overlapping events, a missing
file and `--no-audio`. These are manual checks, not results already established
by this guide.

Do not call native unload functions from the client. `Gui::close()` releases
resources before closing their native contexts. See the
[GUI overview](../gui.md) and [Settings](../settings.md) for related behavior.
