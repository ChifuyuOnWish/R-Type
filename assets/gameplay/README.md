# Notebook gameplay sprites

The drawings are rendered on transparent canvases. The notebook texture is
drawn separately, so no torn sheets move with the entities.

| File | Size | Source and use |
| --- | --- | --- |
| `../player/player_plane.png` | 1024 x 128 | Original four player drawings. A render shader hides the paper while preserving the pencil marks, and the client applies a steering tilt. |
| `basic.png` | 384 x 384 | Redrawn from the team's basic enemy. A small rocking motion is applied in flight. |
| `tank.png` | 384 x 384 | Redrawn from the team's tank enemy. |
| `bat-flight.png` | 1024 x 640 | Four 512 x 320 cells: closed mouth in the top row, open mouth in the bottom row. Both rows have wings extended and lowered. |
| `fireball.png` | 384 x 256 | Redrawn from the supplied fireball. Enemy shots face left; allied shots face right. |

The bat alternates wing positions every 120 ms. Only the bat identified by
a server shot event uses the open mouth frames, for 200 ms. Frames share the
same center and padding. Animation does not change hitboxes or network state.
Monster linework is black, with transparent interiors outside the ink. The bat's graphite lines are reinforced at draw time. Only bats have an enemy fireball weapon, in Campaign and Infinite.

These PNGs were generated from the supplied team drawings, then fitted into
the fixed-size transparent canvases expected by the client.
