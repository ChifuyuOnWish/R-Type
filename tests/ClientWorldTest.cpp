#undef NDEBUG
#include "ClientWorld.hpp"

#include "ClientComponents.hpp"
#include "EntityType.hpp"
#include "components/Components.hpp"
#include "systems/RenderSystem.hpp"

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

    constexpr float step = 0.005F;
    motion.update(step);
    assert(near(find(motion, 1).x, 100.0F + 120.0F * step));
    assert(near(find(motion, 1).y, 200.0F - 60.0F * step));

    motion.update(0.5F);
    assert(near(find(motion, 1).x, 100.0F + 120.0F * 0.05F));
    assert(near(find(motion, 1).y, 200.0F - 60.0F * 0.05F));
    motion.update(0.5F);
    assert(near(find(motion, 1).x, 100.0F + 120.0F * 0.05F));

    client::ClientWorld smoothing;
    const auto player = static_cast<std::uint16_t>(proto::EntityType::Player1);
    smoothing.spawn(spawn(1, player));
    const auto draw = [&] {
        rtype::gui::RenderScene scene;
        client::addWorldSprites(scene, smoothing.registry(), {{1, "Player one"}});
        return scene;
    };
    const auto initial = draw();
    smoothing.applySnapshot({1, {position(1, 30.0F, 20.0F)}});
    const auto corrected = draw();
    assert(near(initial.sprites.front().destinationRect.pos.x,
                corrected.sprites.front().destinationRect.pos.x));
    smoothing.update(0.016F);
    const auto advanced = draw();
    const auto& label = advanced.sceneTexts.front();
    assert(label.text == "Player one");
    assert(label.color.red == 0 && label.color.green == 0 && label.color.blue == 0);
    assert(advanced.sprites.front().destinationRect.pos.x >
           corrected.sprites.front().destinationRect.pos.x);
    assert(near(advanced.sprites.front().destinationRect.pos.x -
                    corrected.sprites.front().destinationRect.pos.x,
                label.position.x - corrected.sceneTexts.front().position.x));

    client::ClientWorld flying;
    flying.spawn(spawn(8, static_cast<std::uint16_t>(proto::EntityType::EnemyBat)));
    const auto batFrame = [&](float time) {
        rtype::gui::RenderScene scene;
        client::addWorldSprites(scene, flying.registry(), {}, time);
        return scene.sprites.front();
    };
    const auto wingsOut = batFrame(0.0F);
    const auto wingsDown = batFrame(0.13F);
    assert(wingsOut.sourceRect.pos.x == 0.F && wingsDown.sourceRect.pos.x == 512.F);
    assert(wingsOut.sourceRect.pos.y == 0.F && wingsDown.sourceRect.pos.y == 0.F);
    flying.sync().markShot(999);
    assert(batFrame(0.13F).sourceRect.pos.y == 0.F);
    flying.sync().markShot(8);
    assert(batFrame(0.13F).sourceRect.pos.y == 320.F);
    flying.update(0.21F);
    assert(batFrame(0.13F).sourceRect.pos.y == 0.F);
    assert(wingsDown.destinationRect.pos.x == wingsOut.destinationRect.pos.x &&
           wingsDown.destinationRect.pos.y == wingsOut.destinationRect.pos.y);

    client::ClientWorld projectiles;
    projectiles.spawn(spawn(1, Missile));
    projectiles.spawn(spawn(2, static_cast<std::uint16_t>(proto::EntityType::EnemyMissile)));
    projectiles.spawn(spawn(3, 20));
    projectiles.spawn(spawn(4, 21));
    rtype::gui::RenderScene shots;
    client::addWorldSprites(shots, projectiles.registry(), {});
    assert(shots.sprites.size() == 4);
    for (const auto& shot : shots.sprites) {
        assert(shot.textureId == "fireball");
        assert(shot.rotation == (shot.entityId.index % 2 == 0 ? 0.0F : 180.0F));
    }

    client::ClientWorld rollover;
    rollover.spawn(spawn(1, Enemy));
    rollover.applySnapshot({std::numeric_limits<std::uint32_t>::max(), {position(1, 1.0F, 2.0F)}});
    rollover.applySnapshot({0, {position(1, 3.0F, 4.0F)}});
    assert(find(rollover, 1).x == 3.0F);
    rollover.applySnapshot({std::numeric_limits<std::uint32_t>::max(), {position(1, 5.0F, 6.0F)}});
    assert(find(rollover, 1).x == 3.0F);
}
