#include "ClientEntityFactory.hpp"

#include "EntityType.hpp"

namespace rtype::client {

namespace {

using Type = protocol::EntityType;

constexpr std::uint16_t raw(Type type)
{
    return static_cast<std::uint16_t>(type);
}

constexpr bool isPlayer(std::uint16_t type)
{
    return type >= raw(Type::Player1) && type <= raw(Type::Player4);
}

} // namespace

ClientEntityFactory::ClientEntityFactory()
{
    for (std::uint16_t i = 0; i < 4; i++) {
        _defs[static_cast<std::uint16_t>(raw(Type::Player1) + i)] = {
            "players", {static_cast<float>(i) * 256.F, 0.F, 256.F, 128.F}, 192.F, 96.F, 10};
    }

    _defs[raw(Type::EnemyBasic)] = {"enemy_basic", {0.F, 0.F, 243.F, 256.F}, 122.F, 128.F, 10};
    _defs[raw(Type::EnemyWave)] = {"enemy_wave", {0.F, 0.F, 256.F, 212.F}, 128.F, 106.F, 10};
    _defs[raw(Type::EnemyTank)] = {"enemy_tank", {0.F, 0.F, 251.F, 256.F}, 188.F, 192.F, 10};
    _defs[raw(Type::PlayerMissile)] = {"player-weapons", {215.F, 85.F, 16.F, 12.F}, 48.F, 36.F, 20};
    _defs[raw(Type::EnemyMissile)] = {"enemy-weapons", {136.F, 6.F, 7.F, 6.F}, 21.F, 18.F, 20};
}

bool ClientEntityFactory::populate(engine::Registry& registry, engine::Entity entity,
                                   std::uint16_t type) const
{
    const auto it = _defs.find(type);
    if (it == _defs.end()) {
        return false;
    }
    const Def& def = it->second;
    registry.addComponent(entity,
                          Sprite{def.texture, def.source, def.width, def.height, def.layer});
    if (isPlayer(type)) {
        registry.addComponent(entity,
                              PlayerSlot{static_cast<std::uint8_t>(type - raw(Type::Player1))});
    }
    return true;
}
} // namespace rtype::client
