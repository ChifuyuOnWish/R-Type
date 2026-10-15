# Graphics, audio and input library study

## Requirements

The GUI needs window management, 2D sprites, animation frames, scrolling
backgrounds, text, keyboard input and sound effects. The library must integrate
with C++ and CMake, support Linux and Windows, and remain isolated from
networking and game logic.

## Evaluation criteria

We compare functionality, API design, resource management, integration effort,
platform support, documentation and measured performance. Integration effort
is a qualitative assessment, not a measurement of development time.

## Alternatives

The evaluated versions are raylib 5.5, SFML 3.0.2 and SDL 3.2.8. These are
evaluation baselines, not necessarily the latest releases.

| Criterion | raylib | SFML | SDL3 |
|---|---|---|---|
| API | C functions and structures | C++ classes | C functions and handles |
| 2D rendering | Texture and shape drawing | Sprite, texture and shape classes | Texture rendering |
| Keyboard input | Key state queries | Events and key state queries | Events and keyboard state queries |
| Text | Built-in font loading and drawing | Font and text classes | SDL_ttf or another text renderer |
| Audio | Sound loading and playback | SoundBuffer and Sound classes | Core audio API; SDL_mixer provides higher-level facilities |
| Images | Built-in loaders, depending on configuration | Graphics module | Core BMP loading; SDL_image extends format support |
| Cleanup | Explicit unload functions | Resource-owning C++ objects | Explicit destroy functions |
| Linux and Windows | Supported upstream | Supported upstream | Supported upstream |
| Documentation | Cheatsheet and examples | Tutorials and API reference | API wiki and examples |

Upstream platform support does not establish successful application testing
on both platforms.

## Integration and resource management

### raylib

raylib provides rendering, input, fonts and audio within one library. Its
explicit resource management requires C++ RAII wrappers. Resources must be
released exactly once, including when initialization fails. Graphics resources
must be released before closing the window; sounds must be released before
closing the audio device. System dependencies and enabled asset formats still
require build configuration.

### SFML

SFML provides graphics, window and audio modules through a native C++ API.
Resource-owning objects simplify cleanup. Textures must still outlive sprites
that reference them, and sound buffers must outlive sounds that use them.

### SDL3

The comparison uses core SDL3 with BMP images and WAV audio. SDL_image,
SDL_ttf and SDL_mixer can provide additional image formats, text rendering
and higher-level audio operations. Those extensions add dependencies and
configuration. Resource handles require explicit cleanup through appropriate
C++ wrappers.

## Prototype evaluation

The comparison prototype uses a shared scene with enemies, one player and
24 projectiles. Its adapters implement rendering, keyboard input and a short
sound effect. Benchmark results measure frame throughput; they do not
independently validate audio correctness, complete input behavior or Windows
compatibility.

### Conditions

| Setting | Value |
|---|---|
| Operating system | Linux, kernel 6.8.0-146-generic |
| Architecture | x86_64 |
| Resolution | 1280 x 720 |
| Enemy counts | 100, 1,000 and 5,000 |
| Measurement duration | 15 seconds |
| Warm-up | 2 seconds |
| Repetitions | Three per library and enemy count |
| VSync | Requested off |
| V2 configuration | HUD and audio disabled; fixed ship animation frame |

The measured loop includes input, simulation, rendering and presentation.
Initialization and resource loading are excluded.

### V2 results

Each value is the median of three per-run measurements. The p95 row summarizes
per-run p95 values, not pooled frame samples.

| Metric | SFML | SDL3 | raylib |
|---|---:|---:|---:|
| 100 enemies, FPS | 662.6 | 712.3 | 715.9 |
| 1,000 enemies, FPS | 488.4 | 555.8 | 575.0 |
| 5,000 enemies, FPS | 243.2 | 272.6 | 438.7 |
| 5,000 enemies, mean frame duration | 4.112 ms | 3.669 ms | 2.279 ms |
| 5,000 enemies, p95 frame duration | 12.694 ms | 13.441 ms | 12.278 ms |

raylib achieved the highest median FPS in V2. The largest difference appeared
at 5,000 enemies. At 100 enemies, SDL3 and raylib run ranges overlap.

### Limitations

GPU model, driver, power mode, compiler and optimization flags were not
recorded. The adapters do not enforce identical GPU batching, and no separate
GPU timing was measured. V2 excludes audio playback, HUD drawing and changing
animation frames. The scene is not a complete multiplayer game.

The earlier V1 results differ substantially:

| Metric | SFML | SDL3 | raylib |
|---|---:|---:|---:|
| 5,000 enemies, median FPS | 390.4 | 438.0 | 452.1 |
| 5,000 enemies, median per-run p95 | 3.621 ms | 3.412 ms | 4.416 ms |

Missing environment information prevents a reliable explanation of the
V1/V2 variation. These results support a local evaluation, not a universal
ranking. The tables summarize two local benchmark sessions. Their recorded
conditions and measurement limitations are detailed above. The prototype
source code is not included here.

## Decision

We select raylib for rendering, audio and input.

Our qualitative assessment favors its direct multimedia API and the ability
to cover the required features through one library. C++ wrappers will keep
library-specific types and resource management inside gui/.

Measured rendering throughput supports this choice for the evaluated scene.
It does not establish that raylib is always faster or easier to use.

We accept responsibility for explicit resource cleanup, build configuration
and platform-specific integration. SFML remains a credible alternative for
native C++ resource objects. SDL3 remains a credible alternative for
lower-level control and a custom combination of multimedia facilities.

## Implementation consequences

- Integrate raylib through the existing dependency management workflow.
- Keep raylib types out of public client and shared game interfaces.
- Encapsulate window, rendering, resources, input and audio operations.
- Make resource-owning wrappers non-copyable.
- Report loading failures and unknown resource IDs.
- Validate production builds and runtime behavior on Linux and Windows.
- Complete functional checks for input, audio and resource cleanup.
- Revisit the choice if representative gameplay reveals a limitation.

## References

- [raylib overview](https://www.raylib.com/)
- [raylib 5.5 API](https://github.com/raysan5/raylib/blob/5.5/src/raylib.h)
- [SFML overview](https://www.sfml-dev.org/)
- [SFML 3.0 tutorials](https://www.sfml-dev.org/tutorials/3.0/)
- [SFML sound resource lifetime](https://www.sfml-dev.org/tutorials/3.0/audio/sounds/)
- [SDL3 overview and extensions](https://wiki.libsdl.org/SDL3/FrontPage)

