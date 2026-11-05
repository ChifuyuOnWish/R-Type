## Entity type IDs

Every replicated entity carries an `EntityType` in its spawn message. The server and the
client share the list in `network/protocol/include/EntityType.hpp`. The client maps each ID to a sprite
in `client/src/ClientEntityFactory.cpp`.

Rules:

- An ID never changes once chosen, because it travels over the network.
- Enemy names follow the level data: `enemy_basic` (level JSON, `enemies.json`) is `EnemyBasic` in code.
- Ranges: 1-4 players, 10-13 enemies and bosses, 14-15 projectiles.

| ID | Name | Level key | Description | Client asset |
|----|------|-----------|-------------|--------------|
| 1 | `Player1` | - | Player 1 ship (paper plane) | `players` |
| 2 | `Player2` | - | Player 2 ship | `players` |
| 3 | `Player3` | - | Player 3 ship | `players` |
| 4 | `Player4` | - | Player 4 ship | `players` |
| 10 | `EnemyScreamer` | `enemy_screamer` | screamer enemy | `enemy_screamer` |
| 10 | `EnemySlug` | `enemy_slug` | slug enemy | `enemy_slug` |
| 11 | `EnemyBat` | `enemy_bat` | Flying enemy with a wave movement (bat) | `enemy_bat` |
| 12 | `EnemyTank` | `enemy_tank` | Heavy enemy (spiked ball) | `enemy_tank` |
| 13 | `Boss` | `boss` | Reserved (future bosses) | - |
| 14 | `PlayerMissile` |  | Player shot | `player-weapons` |
| 15 | `EnemyMissile` | - | Enemy shot | `enemy-weapons` |

### Adding a new entity

1. Add the value to `EntityType.hpp` (next free ID in its range).
2. Add the entry to `ClientEntityFactory.cpp` (texture, source rectangle, size, layer).
3. If it is an enemy, add its key to `assets/levels/enemies.json`.
4. Add a row to the table above.
