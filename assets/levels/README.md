# Data-driven levels

Adding a level means adding a JSON file and listing it in `campaign.json`. No code.

```
gameplay/levels/
├── campaign.json        level order, lives, difficulty scaling with player count
├── enemies.json         enemy types and bosses (stats, movement, weapon, drops; boss attacks are code)
├── formations.json      wave shapes: pyramid, line, v, column, snake, pincer...
├── levels/
│   ├── level_01.json    minimal template: waves only, no terrain, no boss
│   └── level_02.json    full template: waves + terrain + boss
├── schema/              JSON Schemas: editor autocomplete and validation
└── tools/check_levels.py   validates everything, including cross-file references
```

## Why JSON over YAML

**Pick JSON**, with nlohmann/json through CPM.

| | JSON (nlohmann/json) | YAML (yaml-cpp) |
|---|---|---|
| C++ library | header-only, one `CPMAddPackage`, the most used C++ JSON lib | compiled library, slower CI build, less active upstream |
| Errors | strict, one way to write things | indentation mistakes and silent type guesses (`no` → false, `1.10` → 1.1) |
| Editor help | the `$schema` line gives autocomplete, docs on hover and red squiggles in VS Code / CLion | schema support exists but needs a plugin |
| Comments | allowed if you parse with `ignore_comments = true` | native |
| Hand-writing | a bit noisier (quotes, commas) | nicer |

YAML's real advantages are comments and less punctuation. nlohmann/json reads `//` comments when asked, and the schema gives you inline docs, so most of that gap closes, while JSON keeps a header-only dependency and strict parsing. The same lib can also be reused for any other config (keybindings, settings).

```cmake
# cmake/Dependencies.cmake
CPMAddPackage(
    NAME nlohmann_json
    GITHUB_REPOSITORY nlohmann/json
    VERSION 3.11.3
    OPTIONS "JSON_BuildTests OFF"
)
# server/CMakeLists.txt
target_link_libraries(r-type_server PRIVATE nlohmann_json::nlohmann_json)
```

```cpp
auto level = nlohmann::json::parse(file, /*cb*/ nullptr, /*allow_exceptions*/ true, /*ignore_comments*/ true);
```

## The format

### Waves

```json
{ "formation": "pyramid", "roster": { "bydo_basic": 0.5, "bydo_wave": 0.3, "bydo_tank": 0.2 }, "y": "random", "delay": 3.0, "waitForClear": true }
```

- `formation` is your "pattern". I renamed it because the engine already has `EnemyPattern` (straight, sine...) for how an enemy *moves*; "formation" is the *shape* the group spawns in. Both exist in the files: formation per wave, movement per enemy type.
- `roster`: each slot of the formation picks one enemy by weight. Weights are normalised, so `{ "a": 2, "b": 1 }` works too.
- `y`: anchor height in world units (0 to 1080), `"center"` or `"random"`. Waves always enter from the right, as now.
- `delay`: seconds after the previous wave started, or after it was **cleared** when `waitForClear` is true. Mixing both gives pressure (timed) and breathing room (cleared).
- `repeat` / `repeatDelay`: send the same wave several times.

### Formations are data too

A formation is a list of `[x, y]` offsets from the anchor, so a new shape is a JSON edit, not a new `case` in C++:

```json
"pyramid": { "slots": [[0, 0], [90, -70], [90, 70], [180, -140], [180, 0], [180, 140]] }
```

`slotDelay` spawns slots one after the other; with identical offsets it makes a snake that follows one path.

### Level

```json
{ "name": "...", "background": "...", "music": "...", "seed": 1, "waves": [ ... ], "terrain": null, "boss": null }
```

`terrain` and `boss` take `null` when empty. With an empty `waves` list the level goes straight to terrain and boss (a boss-rush stage).

### Terrain

An ASCII map plus a legend, readable and editable in any text editor:

```json
"terrain": {
  "startAt": 4.0, "scrollSpeed": 120, "tileSize": 60,
  "tiles": {
    "#": { "type": "rock",   "contactDamage": 1, "solid": true },
    "x": { "type": "crate",  "contactDamage": 1, "solid": true, "health": 3 },
    "^": { "type": "spikes", "contactDamage": 2, "solid": false }
  },
  "map": [ "######...", "    ####...", ... ]
}
```

- 18 rows of 60 units fill the 1080-high world exactly. The map scrolls left at `scrollSpeed` from `startAt`.
- `solid` tiles stop missiles; non-solid ones (spikes, gas) only hurt on contact.
- `health` makes a tile destructible: `level_02` has a crate wall the players must shoot through.

### Boss

`"boss": { "id": "dobkeratops", "delay": 3.0 }` comes in 3 s after the last wave is cleared. `enemies.json` holds the boss's tunable numbers: health, hitbox, entry movement, and a weak point that takes extra damage (the classic R-Type rule). Its attack patterns and phases are code, one class per boss, registered under the same id (see `../server-refactor.md`).

## How it maps onto the current code

- `WaveSystem` (from the gameplay fix) keeps its job but reads a `Level` struct instead of spawning a Bydo every 1 to 2 s. Each slot becomes one `_factory.create(...)` with `ComponentParams` filled from the enemy entry, so `health`, `hitbox`, `speed`, `movement` and `weapon` map almost 1:1 onto what `GameSystems.cpp` hard-codes today.
- Terrain tiles become ordinary entities with `Position`, `Velocity{-scrollSpeed, 0}`, `Hitbox`, `Damage`, a team that hurts players, and `Health` when destructible. The existing `CollisionSystem` / `DamageSystem` and `ENTITY_SPAWN` replication then work unchanged. Spawn a column only when it is about to enter the screen, not the whole map at load, to keep the entity count and the network traffic low.
- Only the server reads these files. Enemies, bosses, tiles and projectiles name a `type` from `../entity_types.json` (`assets/entity_types.json` in the repo). That file gives each type the id sent in `ENTITY_SPAWN`, plus visual fields (texture, frames, fps) that only the client reads. The server loader fails on an unknown type or a duplicate id.
- Parse into plain structs once at load (`from_json` overloads), validate, and fail with `level_02.json: waves/3: unknown enemy 'bydo_tnak'`-style messages. Never read JSON in the game loop.

## Checking the files

```sh
pip install jsonschema
python3 gameplay/levels/tools/check_levels.py gameplay/levels
```

It checks every file against its schema and the cross-file references (unknown enemy, formation or boss, uneven map rows, undefined tile characters, a map taller than the world). Run it in CI until the server's loader does the same, then keep it as a `--check-levels` flag on `r-type_server` and a unit test that loads every level.

## Other ideas

Roughly in order of value for effort:

1. **Seeded randomness.** `seed` makes roster picks and random heights reproducible: same level, same run. That makes bugs replayable and lets the tests assert "level 1 spawns 21 enemies".
2. **Scaling with player count.** `campaign.json` → `scaling.perExtraPlayer` raises enemy health, enemy count and boss health per extra player, so 1 and 4 players both feel right without separate levels.
3. **Power-up drops.** `drops` on an enemy type (`{ "force_pod": 0.15 }`) puts loot in the data too, ready for the Force pod and speed-ups.
4. **Hot reload in dev.** Watch the level files and restart the current level on change; tuning a wave then takes seconds.
5. **Level select / `--level` flag** on the server to jump to one level while designing it.
6. **Checkpoints.** A `"checkpoint": true` flag on a wave, where dead players respawn.
7. **Enemies on terrain.** A tile kind that spawns a stationary enemy (`"enemy": "bydo_turret"`), for ceiling and floor turrets.
8. **Events in the wave list.** Entries like `{ "event": "music", "id": "boss_theme" }` or `{ "event": "scrollSpeed", "value": 200 }` for set pieces.
9. **A tiny level editor** later: since the map is a grid of characters, even a web page that edits it is a small project, and it is a good bonus for the defense.
