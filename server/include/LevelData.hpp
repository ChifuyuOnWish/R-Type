#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace rtype::server::level {

struct Vec2
{
    float x = 0.F;
    float y = 0.F;
};

enum class Movement
{
    Straight,
    Sine,
    Dive,
    Follow,
    Stationary
};
enum class Aim
{
    Forward,
    NearestPlayer
};

struct WeaponDef
{
    std::string projectile;
    float cooldown = 1.F;
    int damage = 1;
    Aim aim = Aim::Forward;
};

struct Drop
{
    std::string item;
    float chance = 0.F;
};

struct EnemyDef
{
    std::string id;
    std::string sprite;
    int health = 1;
    int contactDamage = 1;
    Vec2 hitbox;
    float speed = 0.F;
    Movement movement = Movement::Straight;
    float amplitude = 0.F; // sine only
    float frequency = 0.F; // sine only
    std::optional<WeaponDef> weapon;
    int score = 0;
    std::vector<Drop> drops;
};

struct BossEntry
{
    std::string from = "right";
    float stopAtX = 1450.F;
    float duration = 3.F;
};

struct WeakPoint
{
    Vec2 offset;
    Vec2 hitbox;
    float damageMultiplier = 1.F;
};

// Only the tunable numbers: each boss's attacks live in its own IBoss class.
struct BossDef
{
    std::string id;
    std::string sprite;
    int health = 1;
    int contactDamage = 1;
    Vec2 hitbox;
    BossEntry entry;
    std::optional<WeakPoint> weakPoint;
    int score = 0;
};

struct Formation
{
    std::string id;
    std::vector<Vec2> slots;
    float slotDelay = 0.F;
};

struct RosterEntry
{
    const EnemyDef* enemy = nullptr;
    float weight = 0.F;
};

struct YAnchor
{
    enum class Kind
    {
        Value,
        Center,
        Random
    };
    Kind kind = Kind::Random;
    float value = 0.F;
};

struct Wave
{
    const Formation* formation = nullptr;
    std::vector<RosterEntry> roster;
    YAnchor y;
    float delay = 0.F;
    bool waitForClear = false;
    int repeat = 1;
    float repeatDelay = 2.F;
};

struct TileDef
{
    std::string sprite;
    int contactDamage = 1;
    bool solid = true;
    std::optional<int> health;
};

struct Terrain
{
    float startAt = 0.F;
    float scrollSpeed = 0.F;
    float tileSize = 60.F;
    std::map<char, TileDef> tiles;
    std::vector<std::string> map;
};

struct BossRef
{
    const BossDef* boss = nullptr;
    float delay = 3.F;
};

struct Level
{
    std::string file;
    std::string name;
    std::string background;
    std::string music;
    std::optional<std::uint32_t> seed;
    std::vector<Wave> waves;
    std::optional<Terrain> terrain;
    std::optional<BossRef> boss;
};

struct Scaling
{
    float enemyHealth = 0.F;
    float enemyCount = 0.F;
    float bossHealth = 0.F;
};

struct GameData
{
    std::map<std::string, EnemyDef> enemies;
    std::map<std::string, BossDef> bosses;
    std::map<std::string, Formation> formations;
    std::vector<Level> levels; // campaign order
    int lives = 3;
    Scaling scaling;

    GameData() = default;
    GameData(const GameData&) = delete;
    GameData& operator=(const GameData&) = delete;
    GameData(GameData&&) = default;
    GameData& operator=(GameData&&) = default;
};

} // namespace rtype::server::level
