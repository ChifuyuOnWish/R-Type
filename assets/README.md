# Assets

Assets are grouped by their main intended use:

| Directory | Content |
|---|---|
| player/ | Player ships, variants, weapons and associated effects |
| enemy/ | Enemy sprite sheets and associated effects |
| boss/ | Large enemy sheets and associated parts or effects |
| projectiles/ | Dedicated projectile and beam sheets |
| effects/ | Dedicated explosion sheets |
| audio/ | Generated placeholder effects used by server gameplay events |

Names describe the artwork for this project's asset organization. They are not
verified original character names or gameplay classifications. Each original
sheet is preserved as one GIF; mixed sheets retain their projectiles and effects.

## Loading

Paths are relative to the assets directory beside the executable:

```cpp
gui.resources().loadTexture("player-sheet", "player/ship-and-effects.gif");
```

The texture ID is independent of its filename. Moving a file changes its path,
not the ID used in drawing data. Keep filename capitalization consistent on
Linux and Windows.

Load each sheet once and select individual frames through source rectangles
when rendering. Each GIF is a static sprite sheet. Animations use different regions
of the same texture.

## Resource lifetime

Load textures and fonts after opening the GUI. Repeating an ID with the same
normalized path and font size reuses the resource. Conflicting registrations
throw. Different IDs are independent registrations.

Missing files, invalid resources and unknown IDs produce exceptions. Failed
loads do not enter the cache. A fallback to raylib's default font is treated
as a failed custom-font load.

Gui clears resources before closing the window. A standalone ResourceManager
must be cleared or destroyed before its graphics context closes.

## Audio effects

`audio/demo-shot.wav` and `audio/demo-hit.wav` are generated sine-wave effects
used as placeholder gameplay effects. The client plays them from server events;
they are separate from the supplied sprite collection. See
[audio documentation](../docs/audio.md) and the
[resource how-to](../docs/how-to/add-gui-resource.md).

## Sources and licenses

The sprite sheets come from the asset collection provided to the team.
No license information was included in the supplied archive.
