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
    Stationary,
    Flight,
    Crawl
};
enum class Aim
{
    Forward,
    NearestPlayer
};

struct WeaponDef
{
    std::string projectile;
    std::uint16_t projectileNetType = 0;
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
    std::string type;
    std::uint16_t netType = 0;
    int health = 1;
    int contactDamage = 1;
    Vec2 hitbox;
    float speed = 0.F;
    Movement movement = Movement::Straight;
    float amplitude = 0.F;
    float frequency = 0.F;
    float homing = 0.F;
    bool onCeiling = false;
    float stopMinX = 0.F;
    float stopMaxX = 0.F;
    float hold = -1.F;
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
    std::string type; // optional: what clients draw there; empty = hitbox only
    std::uint16_t netType = 0;
    Vec2 offset; // from the body's centre
    Vec2 hitbox;
    float damageMultiplier = 1.F;
};

// A named extra piece a boss class asks for (a boss with drones would ask for "drone"). The
// class decides how many there are and how they move; the data gives the numbers.
struct BossPartDef
{
    std::string type;
    std::uint16_t netType = 0;
    int health = 1;
    int contactDamage = 1;
    Vec2 hitbox;
};

struct BossDef
{
    std::string id;
    std::string name; // shown above the health bar; the id when empty
    std::string type;
    std::uint16_t netType = 0;
    int health = 1;
    int contactDamage = 1;
    Vec2 hitbox;
    BossEntry entry;
    std::optional<WeakPoint> weakPoint;
    float bodyDamageMultiplier = 0.F; // hits on the body itself: 0 = armour
    // Health fractions where the next phase starts, highest first: [0.65, 0.3] = 3 phases.
    std::vector<float> phases;
    std::string projectile; // entity type of its shots
    std::uint16_t projectileNetType = 0;
    std::map<std::string, BossPartDef> parts;
    std::map<std::string, const EnemyDef*> minions; // points into GameData::enemies
    std::map<std::string, float> tuning;            // checked against the class by BossRegistry
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
    std::string type;
    std::uint16_t netType = 0;
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
    std::map<std::string, std::uint16_t> entityTypes;
    std::map<std::string, EnemyDef> enemies;
    std::map<std::string, BossDef> bosses;
    std::map<std::string, Formation> formations;
    std::vector<Level> levels;
    int lives = 3;
    Scaling scaling;

    GameData() = default;
    GameData(const GameData&) = delete;
    GameData& operator=(const GameData&) = delete;
    GameData(GameData&&) = default;
    GameData& operator=(GameData&&) = default;
};

} // namespace rtype::server::level
