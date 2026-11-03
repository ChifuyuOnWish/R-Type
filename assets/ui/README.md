# Menu artwork

Images in this directory are optional. Missing images are displayed as white
rectangles so the connection and lobby flow can be tested before artwork is ready.
The title is also an image, with a plain temporary text label when it is missing.

Use PNG files, preferably with transparency. Restart the client after adding artwork.

| File | Purpose |
|------|---------|
| `background.png` | Full connection and lobby background |
| `settings-background.png` | Full notebook background for Settings |
| `title.png` | R-Type title; no text is drawn over this image |
| `settings.png` | Supplied gear image, with transparent outer margins removed offline |
| `paper-button.png` | Wide paper for connection fields and long buttons |
| `paper-action.png` | Medium paper for bottom actions |
| `paper-category.png` | Paper for Settings categories, without a red margin |
| `paper-category-selected.png` | Active category paper, with its red margin on the left |
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

The supplied atlas was separated into complete paper pieces by removing only
transparent surrounding space. The red line was repaired locally on copies,
without changing the original transparency. Wider and square variants were
prepared offline by extending the centre and keeping the end pieces; textures
and edge transitions were joined locally. No generated images are used.
Only the active Settings category uses a red margin, already placed on the left
in its image. The gear also has only its transparent outer margins removed.

Record the source and redistribution license of every image before committing it.

## Notebook alignment

Background images fill the 16:9 window; use a 16:9 canvas to avoid stretching.
For the Settings notebook, the first option baseline is at 107/360 of image height
and following lines are spaced by 30/360 of image height. At 1280 x 720 this is
214 pixels and a 60-pixel pitch. Leave the left 23% for categories, the top for
the heading and the bottom for fixed actions and notices. Option text sits just
above each rule. Match these guides when drawing the paper, or adjust
`firstPaperLine` and `paperLineSpacing` in `SettingsMenu.cpp` to the final asset.
The fallback rules and row positions use the same constants.
