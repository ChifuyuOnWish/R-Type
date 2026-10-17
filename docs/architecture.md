# Architecture

> TODO: fill in once the architecture is decided. Keep it high level: subsystems and how they
> interact, not every class.

```mermaid
flowchart LR
    subgraph Client
        Input --> ClientNet[Networking]
        ClientNet --> Render[Rendering]
    end
    subgraph Server
        ServerNet[Networking] --> Logic[Game logic]
        Logic --> ServerNet
    end
    ClientNet <-- UDP --> ServerNet
```

## Subsystems

### Networking

### Game logic

### Rendering

The presentation subsystem lives in `gui/`, behind the `IGui` interface.
Its presentation types are defined in `GuiTypes.hpp` without exposing raylib types.

The client owns the main loop and supplies a `RenderScene` containing sprites
and text. The GUI renders this data without handling network packets or
gameplay rules.

`Gui` coordinates window management, rendering and visual resources.
`Window` manages the window lifecycle and frame timing. `Renderer` draws the
provided scene, while `ResourceManager` owns textures and fonts and releases
them before the window closes.

The client can enable and configure a scrolling starfield. The GUI updates its
visual offset using elapsed time and repeats the background across the current
window size without scaling entity coordinates.

Input mapping and audio playback are not implemented yet.
