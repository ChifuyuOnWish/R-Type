# Menu artwork

Images in this directory are optional. Missing images are displayed as white
rectangles so the connection and lobby flow can be tested before artwork is ready.
The title is also an image, with a plain temporary text label when it is missing.

Use PNG files, preferably with transparency. Restart the client after adding artwork.

| File | Purpose |
|------|---------|
| `background.png` | Full connection and lobby background |
| `game-background.png` | Notebook texture with slightly wavy rules and a mirrored half for a seamless horizontal loop |
| `settings-background.png` | Full notebook background for Settings |
| `title.png` | R-Type title; no text is drawn over this image |
| `settings.png` | Original hand-drawn gear design isolated as transparent graphite linework |
| `paper-button.png` | Wide paper for connection fields and long buttons |
| `paper-action.png` | Medium paper for bottom actions |
| `paper-category.png` | Paper for Settings categories, without a red margin |
| `paper-small.png` | Short buttons and the top-left FPS badge |
| `paper-mode.png` | Larger game-mode buttons |
| `paper-square.png` | Small square controls |
| `name-field.png` | Player-name field background |
| `ip-field.png` | Server IPv4 field background |
| `port-field.png` | Port field background |
| `connect.png` | Connect button background |
| `campaign.png` | Campaign button background or illustration |
| `infinite.png` | Infinite button background or illustration |
| `start.png` | Start button background |
| `cancel.png` | Cancel button background |
| `retry.png` | Retry button background |
| `back.png` | Return to the connection form |
| `disconnect.png` | Disconnect button background |
| `quit.png` | Quit button background |
| `previous.png`, `next.png` | Level navigation button backgrounds |
| `save.png`, `reset.png` | Settings bottom action backgrounds |
| `settings-display.png`, `settings-audio.png` | Settings category backgrounds |
| `settings-controls.png`, `settings-accessibility.png` | Settings category backgrounds |
| `input.png` | Optional fallback for any missing field background |
| `button.png` | Optional fallback for missing action-button backgrounds |

Labels and editable values are drawn separately using the menu font. Do not bake
those labels into button or field backgrounds. Keep their central area light so
black text remains readable. The disabled state uses a gray tint and disabled
controls do not accept interaction. The selected mode is also identified by text.

Every image is drawn in full, uniformly scaled and centred in its existing
control area. The client does not crop paper edges, select regions from an atlas,
rotate paper or stretch textures to fill controls. Generic controls choose the
paper variant with the closest aspect ratio. Their full control rectangle remains
clickable.

The generic paper variants and notebook textures were generated from
the project's notebook art direction. Transparent sheets were exported as
individual PNGs without stretching the paper shapes. The UI draws a red mark only on the active Settings category. PNG textures use smooth sampling
when displayed at a different size. The Settings gear shares one size and
position across gameplay and the frontend.

## Notebook alignment

Background images fill the 16:9 window; use a 16:9 canvas to avoid stretching.
For the Settings notebook, the first option baseline is at 4/12 of image height
and following lines are spaced by 30/360 of image height. At 1280 x 720 this is
240 pixels and a 60-pixel pitch. Leave the left 23% for categories, the top for
the heading and the bottom for fixed actions and notices. Option text sits just
above each rule. Match these guides when drawing the paper, or adjust
`firstPaperLine` and `paperLineSpacing` in `SettingsMenu.cpp` to the final asset.
The fallback rules and row positions use the same constants.
