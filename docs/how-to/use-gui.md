# Use the GUI

## Build and run the existing client

Install the platform dependencies listed in the repository README first.
Run commands from the repository root.

On Linux:

```sh
cmake --preset release
cmake --build --preset release
ctest --preset release
```

Start the server in one terminal:

```sh
./r-type_server 4242
```

Start a client in another terminal:

```sh
./r-type_client --connect 127.0.0.1:4242 --name Goldy
```

On Windows, use Developer PowerShell:

```powershell
cmake --preset windows
cmake --build --preset windows
ctest --preset windows
```

Then start the executables in separate terminals:

```powershell
.\r-type_server.exe 4242
```

```powershell
.\r-type_client.exe --connect 127.0.0.1:4242 --name Goldy
```

Without `--connect`, the client shows a background and Settings, without creating
local players or enemies. F1, Escape and the Settings button open the menu. See
[Settings](../settings.md) for rebinding, volume, text size and saving.

The current client also accepts `--settings path`, `--no-audio`, `--mute` and
`--volume 0..1`. `--name` selects the name sent during connection. The client
reports connection failures and disconnections through status feedback.

## Write a GUI consumer

Link the consumer target to `rtype::gui`. The existing executable also links
the network and protocol targets because it handles multiplayer messages.
Keep raylib and GLFW includes out of the consumer.

The following complete example shows the presentation lifecycle using one
existing sprite sheet. It illustrates API usage; the production client should
continue to build its scene from server state.

```cpp
#include "Gui.hpp"

int main()
{
    rtype::gui::Gui gui;
    rtype::gui::GuiConfig config;
    config.width = 1920;
    config.height = 1080;
    gui.open(config);
    gui.resources().loadTexture("players", "player/ship-variants.gif");

    rtype::gui::RenderScene scene;
    scene.worldSize = {1920.0F, 1080.0F};
    scene.background.enabled = true;

    rtype::gui::SpriteDraw ship;
    ship.entityId = {1, 0};
    ship.textureId = "players";
    ship.sourceRect = {{67.0F, 2.0F}, 32.0F, 15.0F};
    ship.destinationRect = {{100.0F, 100.0F}, 96.0F, 45.0F};
    scene.sprites.push_back(ship);

    rtype::gui::IGui& presentation = gui;
    while (!presentation.shouldClose()) {
        const auto actions = presentation.pollActions();
        (void)actions;
        // The client consumes actions and updates scene data here.
        presentation.render(scene);
    }
    presentation.close();
}
```

Submit a complete current scene each frame. Source rectangles select image
pixels; destinations use world coordinates when `worldSize` is set. HUD text
belongs in `overlayTexts`, while names and world labels belong in `sceneTexts`.

## Integrate client events

Use `client/src/main.cpp` as the concrete integration reference:

1. Decode server messages in registered `ClientNetwork` handlers.
2. Apply spawns, destruction and snapshots to `ClientWorld`; do not create
   unknown entities from a snapshot.
3. Keep received player names separately from sprite selection.
4. Convert validated server gameplay events into sound IDs registered during
   startup. Call `playAudio()` once with that frame's events.
5. Send held actions through `InputMessage::serialize()` with an increasing
   client tick. Settings returns released/neutral actions while open.
6. Supply connection status through `RenderScene::feedback`, and continue
   polling and rendering until the user quits.

Do not decide hits, deaths or score changes in the GUI. The server owns those
results. See the [module overview](../gui.md), [protocol](../protocol.md) and
[audio reference](../audio.md).
