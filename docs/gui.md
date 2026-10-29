# GUI and client presentation

The client converts server state into presentation data. The GUI displays that
data, samples local input and owns the window, resources, audio and Settings.
It does not decode network packets or decide movement, damage or scores.

## Responsibilities and code locations

Paths below are relative to the repository root.

| Area | Location | Responsibility |
|------|----------|----------------|
| Client coordination | `client/src/main.cpp`, `ClientNetwork.hpp`, `ClientWorld.hpp` | Decode server events, maintain visible entities and player names, send inputs, build scenes and sound events. |
| Public GUI contract | `gui/src/IGui.hpp`, `gui/include/GuiTypes.hpp` | Define lifecycle, action states, scenes, status feedback, preferences and sound events without native dependency types. |
| Window and rendering | `gui/src/Gui.cpp`, `Window.cpp`, `Renderer.cpp`, `ScrollingBackground.cpp`, `StatusPresenter.cpp` | Manage the frame, window, world scaling, draw order, starfield and status pages. |
| Visual resources | `gui/src/ResourceManager.cpp`, `gui/src/detail/ResourceManagerData.hpp` | Own native textures and fonts and resolve IDs used in scene data. |
| Input and preferences | `gui/src/InputReader.cpp`, `InputMapper.cpp`, `SettingsMenu.cpp`, `SettingsStore.cpp` | Read keyboard/mouse/gamepad state, map actions, edit preferences and persist applied settings. |
| Audio | `gui/src/AudioSystem.cpp` | Own the device, sound resources and overlapping playback voices. |

Native raylib types stay in implementation files and the private resource data
header. `detail/` contains implementation data shared by the resource manager
and renderer; it is not part of the client contract. GLFW calls for the native
aspect-ratio constraint stay in `Window.cpp`; controller mapping detection stays
in `InputReader.cpp`. The project uses raylib's bundled
GLFW headers and symbols rather than installing a second GLFW library.

`IGui` is sufficient for frame processing. Concrete `Gui::resources()` and
`Gui::audio()` provide resource registration and error reporting before use.

## Frame flow

1. Call `pollActions()` once to obtain held, pressed and released actions.
2. Drain network events on the client thread and update `ClientWorld` and names.
3. Convert server gameplay events into `SoundEvent` values and call `playAudio()`.
4. Serialize held actions into an `InputMessage` while connected.
5. Build the current `RenderScene`, including connection feedback, and call
   `render()` even when Settings or a status page is visible.

These are the steps used in `client/src/main.cpp`. Network transport runs on its
own thread; scene construction and native GUI calls remain on the client thread.
Keyboard and the selected controller share the same action contract. Controller
mappings remain implementation details of the GUI. Opening Settings releases gameplay actions without stopping network updates.
Native input events collected by rendering are sampled on the next iteration.

See [use the GUI](how-to/use-gui.md) for a minimal consumer and
[controls](controls.md) for input transitions and rebinding.

## Presentation data

| Data | Meaning |
|------|---------|
| `SpriteDraw` | Texture ID, source region, destination rectangle, origin, rotation, tint, layer and entity identifier. |
| `sceneTexts` | Text attached to the game world, such as player names. |
| `overlayTexts` | Text in window pixels, such as the title and FPS counter. |
| `worldSize` | Logical world dimensions; `{0, 0}` keeps pixel coordinates. |
| `background` | Whether the starfield is enabled, its color and scrolling speed. |
| `feedback` | None, Connecting, Rejected, Disconnected or Error, with a message. |
| `SoundEvent` | ID of a sound previously registered through `AudioSystem`. |

The renderer retains no entities between frames. Submit an entity every frame
while it should be visible; omitting it removes it immediately. The client
keeps network entity IDs in `GuiEntityId::index` and uses generation zero because
the current wire IDs identify entities for the lifetime of the game.

Sprites use top-left destination coordinates. Server positions are entity
centers, so `main.cpp` subtracts half the sprite's width and height when building
the destination rectangle. Player types select sprite variants; connected and
joined-player messages provide the labels. Until a name is known, the label is
`Player`.

## Window, coordinates and draw order

The current client requests 1920 x 1080. `Window` applies a minimum of 640 x 360
and configures the resizable native window to preserve 16:9 once at creation.
It does not call `SetWindowSize()` every frame. Escape is reserved for Settings;
use the close button or Quit application to exit.

With `worldSize = {1920, 1080}`, `Gui` uniformly scales sprite destinations,
origins and scene text to fit the current window and centers the result. Source
texture regions remain unchanged. Overlay positions remain in window pixels.
The player's text-size preference is applied on top of world scaling for scene
text and directly to HUD text. Client HUD positions must allow larger fonts.

The world rendering order is:

1. Clear color and starfield.
2. Sprites in ascending layer order, preserving submission order within a layer.
3. Scene text.
4. Optional full-window scene tint.
5. UI rectangles, then overlay text, including the Settings button.

Settings replaces the game view when open. Otherwise, non-None status feedback
replaces it with a readable status page. Neither case stops the client loop.
The starfield uses elapsed frame time for scrolling. The HUD FPS count is an
average refreshed approximately twice per second, separate from the FPS target.

## Resource and audio lifecycle

Open the GUI before loading textures, fonts or sound effects. Resource IDs are
independent of filenames. Visual loading errors throw; audio loading failures
return false with `getLastError()`. Report the error before continuing.

`Gui::close()` closes audio, clears visual resources and then closes the window.
Do not close the graphics context while resources using it remain alive. A
standalone resource manager must be cleared or destroyed first.

The client currently maps server Shoot events to `shot` and Hit, Kill and
PlayerDeath to `hit`. It uses the two generated placeholder WAV files already
in `assets/audio/`. No action text is displayed. See [audio](audio.md) for mute,
voice limits and behavior while Settings is open, and
[add a GUI resource](how-to/add-gui-resource.md) for registration examples.

## Preferences and validation

The GUI owns local keyboard, audio and visual preferences. Apply and save is
the persistence boundary; Back discards unapplied edits. See
[Settings](settings.md) for paths, supported values and failure handling, and
[accessibility](accessibility.md) for current features and limitations.

Automated GUI tests run without a physical window or audio device. They cover
the interface, resource errors, input mapping, Settings, status layouts,
starfield motion, rendering order and native aspect-ratio configuration through
fake boundaries. They cannot establish hardware readability, audio quality or
successful multiplayer rendering on Linux and Windows; those need manual checks.

