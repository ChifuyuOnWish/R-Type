#undef NDEBUG
#include "ClientWorld.hpp"

#include "ClientComponents.hpp"
#include "EntityType.hpp"
#include "components/Components.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace proto = rtype::protocol;
namespace client = rtype::client;
namespace engine = rtype::engine;

namespace {

constexpr std::uint16_t Enemy = static_cast<std::uint16_t>(proto::EntityType::EnemyBasic);
constexpr std::uint16_t Missile = static_cast<std::uint16_t>(proto::EntityType::PlayerMissile);
constexpr std::uint16_t Unknown = 60000;

struct Found
{
    bool exists = false;
    bool hasSprite = false;
    float x = 0.0F;
    float y = 0.0F;
    float dx = 0.0F;
    float dy = 0.0F;
};

proto::EntitySpawnMessage spawn(std::uint32_t id, std::uint16_t type)
{
    return {id, type, proto::Fixed::fromFloat(10.0F), proto::Fixed::fromFloat(20.0F)};
}

proto::SnapshotEntry position(std::uint32_t id, float x, float y)
{
    return {id, proto::Fixed::fromFloat(x), proto::Fixed::fromFloat(y), {}, {}};
}

proto::SnapshotEntry moving(std::uint32_t id, float x, float y, float vx, float vy)
{
    return {id, proto::Fixed::fromFloat(x), proto::Fixed::fromFloat(y),
            proto::Fixed16::fromFloat(vx), proto::Fixed16::fromFloat(vy)};
}

Found find(client::ClientWorld& world, std::uint32_t id)
{
    Found found;
    auto& registry = world.registry();
    registry.each<client::NetworkId, engine::Position, engine::Velocity>(
        [&](engine::Entity entity, client::NetworkId& net, engine::Position& p,
            engine::Velocity& v) {
            if (net.id != id)
                return;
            found = {true, registry.hasComponent<client::Sprite>(entity), p.x, p.y, v.dx, v.dy};
        });
    return found;
}

std::size_t count(client::ClientWorld& world)
{
    std::size_t total = 0;
    world.registry().each<client::NetworkId>([&](engine::Entity, client::NetworkId&) { ++total; });
    return total;
}

bool near(float a, float b)
{
    return std::fabs(a - b) < 0.5F;
}

} // namespace

int main()
{
    client::ClientWorld world;
    world.spawn(spawn(1, Enemy));
    world.spawn(spawn(2, Missile));
    assert(count(world) == 2);
    assert(find(world, 1).hasSprite);
    assert(find(world, 2).x == 10.0F);

    world.spawn(spawn(3, Unknown));
    assert(!find(world, 3).exists);
    assert(count(world) == 2);

    world.applySnapshot({10, {position(1, 100.0F, 200.0F), position(99, 1.0F, 2.0F)}});
    assert(count(world) == 2);
    assert(find(world, 1).x == 100.0F);
    assert(find(world, 1).y == 200.0F);
    assert(find(world, 2).x == 10.0F);

    world.applySnapshot({10, {position(2, 300.0F, 400.0F)}});
    assert(find(world, 1).x == 100.0F);
    assert(find(world, 2).x == 300.0F);

    world.applySnapshot({9, {position(1, 0.0F, 0.0F)}});
    assert(find(world, 1).x == 100.0F);

    world.spawn(spawn(1, Enemy));
    assert(count(world) == 2);
    assert(find(world, 1).x == 100.0F);

    world.destroy({1, proto::DestroyCause::Other});
    assert(!find(world, 1).exists);
    world.spawn(spawn(1, Enemy));
    world.applySnapshot({11, {position(1, 50.0F, 60.0F)}});
    assert(!find(world, 1).exists);
    assert(count(world) == 1);

    world.destroy({99, proto::DestroyCause::Other});
    world.spawn(spawn(99, Enemy));
    assert(!find(world, 99).exists);

    client::ClientWorld motion;
    motion.spawn(spawn(1, Enemy));
    motion.applySnapshot({1, {moving(1, 100.0F, 200.0F, 2.0F, -1.0F)}});
    assert(near(find(motion, 1).dx, 120.0F));
    assert(near(find(motion, 1).dy, -60.0F));
    motion.update(0.5F);
    assert(near(find(motion, 1).x, 160.0F));
    assert(near(find(motion, 1).y, 170.0F));

    client::ClientWorld rollover;
    rollover.spawn(spawn(1, Enemy));
    rollover.applySnapshot({std::numeric_limits<std::uint32_t>::max(), {position(1, 1.0F, 2.0F)}});
    rollover.applySnapshot({0, {position(1, 3.0F, 4.0F)}});
    assert(find(rollover, 1).x == 3.0F);
    rollover.applySnapshot({std::numeric_limits<std::uint32_t>::max(), {position(1, 5.0F, 6.0F)}});
    assert(find(rollover, 1).x == 3.0F);
}
