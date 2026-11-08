#include "ClientEntityFactory.hpp"

#include "EntityType.hpp"

namespace rtype::client {

namespace {

using Type = protocol::EntityType;

constexpr std::uint16_t raw(Type type)
{
    return static_cast<std::uint16_t>(type);
}

constexpr std::uint16_t SlugFloor = 13;
constexpr std::uint16_t SlugCeiling = 16;
constexpr std::uint16_t TileRock = 30;
constexpr std::uint16_t TileCrate = 31;
constexpr std::uint16_t TileSpikes = 32;
constexpr float TileSize = 60.F;
constexpr float TileCell = 128.F;
constexpr int TerrainLayer = 5;
// boss_tank in entity_types.json: the tank drawn about 2.7 times bigger, behind the other
// enemies so the minions it rolls out are visible on top of it. Hitbox 380x340 on the server.
constexpr std::uint16_t BossTank = 40;
constexpr float BossTankSize = 520.F;
constexpr int BossLayer = 8;

constexpr bool isPlayer(std::uint16_t type)
{
    return type >= raw(Type::Player1) && type <= raw(Type::Player4);
}

} // namespace

ClientEntityFactory::ClientEntityFactory()
{
    for (std::uint16_t i = 0; i < 4; i++) {
        _defs[static_cast<std::uint16_t>(raw(Type::Player1) + i)] = {
            "players", {0.F, 0.F, 512.F, 256.F}, 160.F, 80.F, 10, SpriteMotion::Plane};
    }

    _defs[raw(Type::EnemyBasic)] = {"enemy_basic",      {0.F, 0.F, 384.F, 384.F}, 122.F, 122.F, 10,
                                    SpriteMotion::Hover};
    _defs[raw(Type::EnemyBat)] = {"enemy_bat", {0.F, 0.F, 512.F, 320.F}, 144.F, 90.F,
                                  10,          SpriteMotion::Bat};
    _defs[SlugFloor] = {"enemy_slug", {0.F, 0.F, 434.F, 443.F}, 118.F, 120.F,
                        10,           SpriteMotion::Slug};
    _defs[SlugCeiling] = _defs.at(SlugFloor);
    _defs[SlugCeiling].flipY = true;
    _defs[TileRock] = {"tiles", {0.F, 0.F, TileCell, TileCell}, TileSize, TileSize, TerrainLayer};
    _defs[TileCrate] = {
        "tiles", {TileCell, 0.F, TileCell, TileCell}, TileSize, TileSize, TerrainLayer};
    _defs[TileSpikes] = {
        "tiles", {2.F * TileCell, 0.F, TileCell, TileCell}, TileSize, TileSize, TerrainLayer};
    _defs[raw(Type::EnemyTank)] = {"enemy_tank", {0.F, 0.F, 384.F, 384.F}, 188.F, 188.F, 10};
    _defs[BossTank] = {"enemy_tank", {0.F, 0.F, 384.F, 384.F}, BossTankSize, BossTankSize,
                       BossLayer,    SpriteMotion::Boss};
    _defs[raw(Type::PlayerMissile)] = {"fireball", {0.F, 0.F, 384.F, 256.F}, 72.F, 48.F,
                                       20,         SpriteMotion::AlliedFire};
    _defs[raw(Type::EnemyMissile)] = {"fireball", {0.F, 0.F, 384.F, 256.F}, 72.F, 48.F,
                                      20,         SpriteMotion::EnemyFire};
    _defs[20] = _defs.at(raw(Type::PlayerMissile));
    _defs[21] = _defs.at(raw(Type::EnemyMissile));
}

bool ClientEntityFactory::populate(engine::Registry& registry, engine::Entity entity,
                                   std::uint16_t type) const
{
    const auto it = _defs.find(type);
    if (it == _defs.end()) {
        return false;
    }
    const Def& def = it->second;
    registry.addComponent(entity, Sprite{def.texture, def.source, def.width, def.height, def.layer,
                                         def.motion, 0.F, def.flipY});
    if (isPlayer(type)) {
        registry.addComponent(entity,
                              PlayerSlot{static_cast<std::uint8_t>(type - raw(Type::Player1))});
    }
    return true;
}
} // namespace rtype::client
