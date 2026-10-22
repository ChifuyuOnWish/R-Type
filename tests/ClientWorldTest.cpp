#undef NDEBUG
#include "ClientWorld.hpp"

#include <cassert>
#include <cstdint>
#include <limits>

namespace proto = rtype::protocol;

proto::EntitySpawnMessage spawn(std::uint32_t id, std::uint16_t type)
{
    return {id, type, proto::Fixed::fromFloat(10.0F), proto::Fixed::fromFloat(20.0F)};
}

proto::SnapshotEntry position(std::uint32_t id, float x, float y)
{
    return {id, proto::Fixed::fromFloat(x), proto::Fixed::fromFloat(y), {}, {}};
}

int main()
{
    rtype::client::ClientWorld world;
    world.spawn(spawn(1, 2));
    world.spawn(spawn(2, 10));

    world.applySnapshot({10, {position(1, 100.0F, 200.0F), position(99, 1.0F, 2.0F)}});
    assert(world.entities().size() == 2);
    assert(world.entities().at(1).type == 2);
    assert(world.entities().at(1).x == 100.0F);
    assert(world.entities().at(1).y == 200.0F);
    assert(world.entities().at(2).x == 10.0F);

    world.applySnapshot({10, {position(2, 300.0F, 400.0F)}});
    assert(world.entities().at(1).x == 100.0F);
    assert(world.entities().at(2).x == 300.0F);
    world.applySnapshot({9, {position(1, 0.0F, 0.0F)}});
    assert(world.entities().at(1).x == 100.0F);

    world.spawn(spawn(1, 2));
    assert(world.entities().at(1).x == 100.0F);
    world.destroy({1, proto::DestroyCause::Other});
    world.applySnapshot({11, {position(1, 50.0F, 60.0F)}});
    assert(!world.entities().contains(1));
    world.destroy({99, proto::DestroyCause::Other});

    rtype::client::ClientWorld rollover;
    rollover.spawn(spawn(1, 1));
    rollover.applySnapshot({std::numeric_limits<std::uint32_t>::max(), {position(1, 1.0F, 2.0F)}});
    rollover.applySnapshot({0, {position(1, 3.0F, 4.0F)}});
    assert(rollover.entities().at(1).x == 3.0F);
    rollover.applySnapshot({std::numeric_limits<std::uint32_t>::max(), {position(1, 5.0F, 6.0F)}});
    assert(rollover.entities().at(1).x == 3.0F);
}
