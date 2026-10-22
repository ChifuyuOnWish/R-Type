#include "level/LevelLoader.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <set>
#include <utility>

namespace rtype::server::level {

using nlohmann::json;

namespace {

constexpr float WorldHeight = 1080.F;

struct Where
{
    std::string file;
    std::string path;

    Where operator/(const std::string& key) const
    {
        return Where{file, path.empty() ? key : path + "/" + key};
    }
    Where operator/(std::size_t index) const { return *this / std::to_string(index); }

    [[noreturn]] void fail(const std::string& what) const
    {
        throw LevelError(file + ": " + (path.empty() ? "(root)" : path) + ": " + what);
    }
};

json readFile(const std::filesystem::path& dir, const std::string& name)
{
    std::ifstream in(dir / name);
    if (!in)
        throw LevelError(name + ": cannot open " + (dir / name).string());
    try {
        return json::parse(in, nullptr, true, true); // allow exceptions, ignore // comments
    } catch (const json::parse_error& e) {
        throw LevelError(name + ": invalid JSON: " + e.what());
    }
}

const json& member(const json& obj, const char* key, const Where& at)
{
    if (!obj.is_object())
        at.fail("expected an object");
    const auto it = obj.find(key);
    if (it == obj.end())
        (at / key).fail("missing");
    return *it;
}

bool has(const json& obj, const std::string& key)
{
    const auto it = obj.find(key);
    return it != obj.end() && !it->is_null();
}

float number(const json& v, const Where& at)
{
    if (!v.is_number())
        at.fail("expected a number");
    return v.get<float>();
}

int integer(const json& v, const Where& at)
{
    if (!v.is_number_integer())
        at.fail("expected an integer");
    return v.get<int>();
}

std::string text(const json& v, const Where& at)
{
    if (!v.is_string())
        at.fail("expected a string");
    return v.get<std::string>();
}

bool boolean(const json& v, const Where& at)
{
    if (!v.is_boolean())
        at.fail("expected true or false");
    return v.get<bool>();
}

float numberOr(const json& obj, const std::string& key, float fallback, const Where& at)
{
    return has(obj, key) ? number(obj[key], at / key) : fallback;
}

int integerOr(const json& obj, const std::string& key, int fallback, const Where& at)
{
    return has(obj, key) ? integer(obj[key], at / key) : fallback;
}

std::string textOr(const json& obj, const std::string& key, std::string fallback, const Where& at)
{
    return has(obj, key) ? text(obj[key], at / key) : std::move(fallback);
}

float atLeast(float value, float min, const Where& at)
{
    if (value < min)
        at.fail("must be at least " + std::to_string(min));
    return value;
}

Vec2 vec2(const json& v, const Where& at)
{
    if (!v.is_array() || v.size() != 2)
        at.fail("expected [x, y]");
    return Vec2{number(v[0], at / 0), number(v[1], at / 1)};
}

Aim aim(const json& obj, const Where& at)
{
    const std::string value = textOr(obj, "aim", "forward", at);
    if (value == "forward")
        return Aim::Forward;
    if (value == "nearest_player")
        return Aim::NearestPlayer;
    (at / "aim").fail("unknown aim '" + value + "'");
}

Movement movement(const json& v, const Where& at)
{
    const std::string type = text(member(v, "type", at), at / "type");
    if (type == "straight")
        return Movement::Straight;
    if (type == "sine")
        return Movement::Sine;
    if (type == "dive")
        return Movement::Dive;
    if (type == "follow")
        return Movement::Follow;
    if (type == "stationary")
        return Movement::Stationary;
    (at / "type").fail("unknown movement '" + type + "'");
}

EnemyDef enemy(const std::string& id, const json& v, const Where& at)
{
    EnemyDef e;
    e.id = id;
    e.sprite = text(member(v, "sprite", at), at / "sprite");
    e.health = integer(member(v, "health", at), at / "health");
    if (e.health < 1)
        (at / "health").fail("must be at least 1");
    e.contactDamage = integerOr(v, "contactDamage", 1, at);
    e.hitbox = vec2(member(v, "hitbox", at), at / "hitbox");
    e.speed = atLeast(number(member(v, "speed", at), at / "speed"), 0.F, at / "speed");
    const json& move = member(v, "movement", at);
    e.movement = movement(move, at / "movement");
    e.amplitude = numberOr(move, "amplitude", 0.F, at / "movement");
    e.frequency = numberOr(move, "frequency", 0.F, at / "movement");
    if (has(v, "weapon")) {
        const json& w = v["weapon"];
        const Where wat = at / "weapon";
        WeaponDef weapon;
        weapon.projectile = text(member(w, "projectile", wat), wat / "projectile");
        weapon.cooldown = number(member(w, "cooldown", wat), wat / "cooldown");
        if (weapon.cooldown <= 0.F)
            (wat / "cooldown").fail("must be above 0");
        weapon.damage = integerOr(w, "damage", 1, wat);
        weapon.aim = aim(w, wat);
        e.weapon = weapon;
    }
    e.score = integerOr(v, "score", 0, at);
    if (has(v, "drops")) {
        for (const auto& [item, chance] : v["drops"].items()) {
            const float c = number(chance, at / "drops" / item);
            if (c < 0.F || c > 1.F)
                (at / "drops" / item).fail("chance must be between 0 and 1");
            e.drops.push_back(Drop{item, c});
        }
    }
    return e;
}

BossDef boss(const std::string& id, const json& v, const Where& at)
{
    BossDef b;
    b.id = id;
    b.sprite = text(member(v, "sprite", at), at / "sprite");
    b.health = integer(member(v, "health", at), at / "health");
    if (b.health < 1)
        (at / "health").fail("must be at least 1");
    b.contactDamage = integerOr(v, "contactDamage", 1, at);
    b.hitbox = vec2(member(v, "hitbox", at), at / "hitbox");
    if (has(v, "entry")) {
        const json& e = v["entry"];
        const Where eat = at / "entry";
        b.entry.from = textOr(e, "from", "right", eat);
        if (b.entry.from != "right" && b.entry.from != "top" && b.entry.from != "bottom")
            (eat / "from").fail("must be right, top or bottom");
        b.entry.stopAtX = numberOr(e, "stopAtX", b.entry.stopAtX, eat);
        b.entry.duration = numberOr(e, "duration", b.entry.duration, eat);
    }
    if (has(v, "weakPoint")) {
        const json& w = v["weakPoint"];
        const Where wat = at / "weakPoint";
        b.weakPoint = WeakPoint{vec2(member(w, "offset", wat), wat / "offset"),
                                vec2(member(w, "hitbox", wat), wat / "hitbox"),
                                numberOr(w, "damageMultiplier", 1.F, wat)};
    }
    b.score = integerOr(v, "score", 0, at);
    return b;
}

Formation formation(const std::string& id, const json& v, const Where& at)
{
    Formation f;
    f.id = id;
    const json& slots = member(v, "slots", at);
    if (!slots.is_array() || slots.empty())
        (at / "slots").fail("expected at least one [x, y]");
    for (std::size_t i = 0; i < slots.size(); ++i)
        f.slots.push_back(vec2(slots[i], at / "slots" / i));
    f.slotDelay = atLeast(numberOr(v, "slotDelay", 0.F, at), 0.F, at / "slotDelay");
    return f;
}

YAnchor yAnchor(const json& wave, const Where& at)
{
    if (!has(wave, "y"))
        return YAnchor{YAnchor::Kind::Random, 0.F};
    const json& y = wave["y"];
    if (y.is_number()) {
        const float value = y.get<float>();
        if (value < 0.F || value > WorldHeight)
            (at / "y").fail("must be between 0 and 1080");
        return YAnchor{YAnchor::Kind::Value, value};
    }
    const std::string kind = text(y, at / "y");
    if (kind == "center")
        return YAnchor{YAnchor::Kind::Center, 0.F};
    if (kind == "random")
        return YAnchor{YAnchor::Kind::Random, 0.F};
    (at / "y").fail("expected a number, \"center\" or \"random\"");
}

Wave wave(const json& v, const GameData& data, const Where& at)
{
    Wave w;
    const std::string formationId = text(member(v, "formation", at), at / "formation");
    const auto f = data.formations.find(formationId);
    if (f == data.formations.end())
        (at / "formation").fail("unknown formation '" + formationId + "'");
    w.formation = &f->second;

    const json& roster = member(v, "roster", at);
    if (!roster.is_object() || roster.empty())
        (at / "roster").fail("expected at least one enemy: weight");
    float total = 0.F;
    for (const auto& [enemyId, weight] : roster.items()) {
        const auto e = data.enemies.find(enemyId);
        if (e == data.enemies.end())
            (at / "roster").fail("unknown enemy '" + enemyId + "'");
        const float wgt = number(weight, at / "roster" / enemyId);
        if (wgt <= 0.F)
            (at / "roster" / enemyId).fail("weight must be above 0");
        w.roster.push_back(RosterEntry{&e->second, wgt});
        total += wgt;
    }
    for (auto& entry : w.roster)
        entry.weight /= total;

    w.y = yAnchor(v, at);
    w.delay = atLeast(numberOr(v, "delay", 0.F, at), 0.F, at / "delay");
    w.waitForClear = has(v, "waitForClear") && boolean(v["waitForClear"], at / "waitForClear");
    w.repeat = integerOr(v, "repeat", 1, at);
    if (w.repeat < 1)
        (at / "repeat").fail("must be at least 1");
    w.repeatDelay = atLeast(numberOr(v, "repeatDelay", 2.F, at), 0.F, at / "repeatDelay");
    return w;
}

Terrain terrain(const json& v, const Where& at)
{
    Terrain t;
    t.startAt = atLeast(numberOr(v, "startAt", 0.F, at), 0.F, at / "startAt");
    t.scrollSpeed = number(member(v, "scrollSpeed", at), at / "scrollSpeed");
    t.tileSize = number(member(v, "tileSize", at), at / "tileSize");
    if (t.scrollSpeed <= 0.F || t.tileSize <= 0.F)
        at.fail("scrollSpeed and tileSize must be above 0");

    const json& tiles = member(v, "tiles", at);
    if (!tiles.is_object())
        (at / "tiles").fail("expected an object");
    for (const auto& [key, def] : tiles.items()) {
        const Where tat = at / "tiles" / key;
        if (key.size() != 1 || key == " ")
            tat.fail("a tile key is one character, and not a space");
        TileDef tile;
        tile.sprite = text(member(def, "sprite", tat), tat / "sprite");
        tile.contactDamage = integerOr(def, "contactDamage", 1, tat);
        tile.solid = !has(def, "solid") || boolean(def["solid"], tat / "solid");
        if (has(def, "health"))
            tile.health = integer(def["health"], tat / "health");
        t.tiles.emplace(key[0], tile);
    }

    const json& rows = member(v, "map", at);
    if (!rows.is_array() || rows.empty())
        (at / "map").fail("expected at least one row");
    for (std::size_t r = 0; r < rows.size(); ++r) {
        std::string row = text(rows[r], at / "map" / r);
        if (!t.map.empty() && row.size() != t.map.front().size())
            (at / "map" / r).fail("all rows must have the same length");
        for (const char c : row)
            if (c != ' ' && t.tiles.find(c) == t.tiles.end())
                (at / "map" / r).fail(std::string("undefined tile '") + c + "'");
        t.map.push_back(std::move(row));
    }
    if (static_cast<float>(t.map.size()) * t.tileSize > WorldHeight)
        (at / "map").fail(std::to_string(t.map.size()) + " rows overflow the 1080-high world");
    return t;
}

Level level(const std::string& file, const json& v, const GameData& data)
{
    const Where at{file, ""};
    Level l;
    l.file = file;
    l.name = text(member(v, "name", at), at / "name");
    l.background = textOr(v, "background", "", at);
    l.music = textOr(v, "music", "", at);
    if (has(v, "seed"))
        l.seed = static_cast<std::uint32_t>(integer(v["seed"], at / "seed"));

    const json& waves = member(v, "waves", at);
    if (!waves.is_array())
        (at / "waves").fail("expected a list");
    for (std::size_t i = 0; i < waves.size(); ++i)
        l.waves.push_back(wave(waves[i], data, at / "waves" / i));

    if (has(v, "terrain"))
        l.terrain = terrain(v["terrain"], at / "terrain");
    if (has(v, "boss")) {
        const json& b = v["boss"];
        const std::string id = text(member(b, "id", at / "boss"), at / "boss" / "id");
        const auto found = data.bosses.find(id);
        if (found == data.bosses.end())
            (at / "boss" / "id").fail("unknown boss '" + id + "'");
        l.boss = BossRef{&found->second, numberOr(b, "delay", 3.F, at / "boss")};
    }
    return l;
}

} // namespace

GameData LevelLoader::load(const std::filesystem::path& dir)
{
    GameData data;

    const json enemies = readFile(dir, "enemies.json");
    const Where eat{"enemies.json", ""};
    for (const auto& [id, def] : member(enemies, "enemies", eat).items())
        data.enemies.emplace(id, enemy(id, def, eat / "enemies" / id));
    if (has(enemies, "bosses"))
        for (const auto& [id, def] : enemies["bosses"].items())
            data.bosses.emplace(id, boss(id, def, eat / "bosses" / id));

    const json formations = readFile(dir, "formations.json");
    const Where fat{"formations.json", ""};
    for (const auto& [id, def] : member(formations, "formations", fat).items())
        data.formations.emplace(id, formation(id, def, fat / "formations" / id));

    const json campaign = readFile(dir, "campaign.json");
    const Where cat{"campaign.json", ""};
    data.lives = integerOr(campaign, "lives", 3, cat);
    if (has(campaign, "scaling") && has(campaign["scaling"], "perExtraPlayer")) {
        const json& s = campaign["scaling"]["perExtraPlayer"];
        const Where sat = cat / "scaling" / "perExtraPlayer";
        data.scaling =
            Scaling{numberOr(s, "enemyHealth", 0.F, sat), numberOr(s, "enemyCount", 0.F, sat),
                    numberOr(s, "bossHealth", 0.F, sat)};
    }
    const json& files = member(campaign, "levels", cat);
    if (!files.is_array() || files.empty())
        (cat / "levels").fail("expected at least one level file");
    for (std::size_t i = 0; i < files.size(); ++i) {
        const std::string file = text(files[i], cat / "levels" / i);
        data.levels.push_back(level(file, readFile(dir, file), data));
    }
    return data;
}

} // namespace rtype::server::level
