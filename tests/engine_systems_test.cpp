#undef NDEBUG
#include "BoundsSystem.hpp"
#include "Components.hpp"
#include "InputSystem.hpp"
#include "MotionSystem.hpp"
#include "Registry.hpp"

#include <cassert>
#include <iostream>
#include <type_traits>

using namespace rtype::engine;

// Components must be plain, copyable data.
static_assert(std::is_trivially_copyable_v<Position>);
static_assert(std::is_trivially_copyable_v<Velocity>);
static_assert(std::is_trivially_copyable_v<Hitbox>);
static_assert(std::is_trivially_copyable_v<Health>);
static_assert(std::is_trivially_copyable_v<Damage>);
static_assert(std::is_trivially_copyable_v<Lifetime>);
static_assert(std::is_trivially_copyable_v<Invincibility>);
static_assert(std::is_trivially_copyable_v<Sprite>);
static_assert(std::is_trivially_copyable_v<Animation>);
static_assert(std::is_trivially_copyable_v<Controllable>);
static_assert(std::is_trivially_copyable_v<PlayerOwner>);
static_assert(std::is_trivially_copyable_v<EnemyBehavior>);
static_assert(std::is_trivially_copyable_v<Weapon>);
static_assert(std::is_trivially_copyable_v<Projectile>);
static_assert(std::is_trivially_copyable_v<Score>);

static void test_defaults()
{
    assert(Position{}.x == 0.F && Position{}.y == 0.F);
    assert(Velocity{}.dx == 0.F);
    assert(Hitbox{}.width == 0.F);
    assert(Health{}.current == 1 && Health{}.max == 1);
    assert(Damage{}.value == 0);
    assert(Lifetime{}.remaining == 0.F);
    assert(Invincibility{}.remaining == 0.F);
    assert(Sprite{}.scale == 1.F && Sprite{}.layer == 0);
    assert(Animation{}.frameCount == 1 && Animation{}.loop);
    assert(!Controllable{}.up && !Controllable{}.shoot);
    assert(PlayerOwner{}.playerId == 0);
    assert(EnemyBehavior{}.pattern == EnemyPattern::Straight);
    assert(Weapon{}.rate == 1.F);
    assert(Projectile{}.owner.isNull());
    assert(Score{}.value == 0 && Score{}.total == 0);
}

static void test_components_can_be_stored_in_registry()
{
    Registry reg;
    const Entity e = reg.createEntity();
    reg.addComponent(e, Position{1.F, 2.F});
    reg.addComponent(e, Velocity{3.F, 4.F});
    reg.addComponent(e, Hitbox{5.F, 6.F});
    reg.addComponent(e, Health{7, 10});
    reg.addComponent(e, Lifetime{8.F});
    reg.addComponent(e, PlayerOwner{3});
    reg.addComponent(e, Projectile{e, 2.F, 1});

    assert(reg.getComponent<Position>(e).y == 2.F);
    assert(reg.getComponent<Velocity>(e).dx == 3.F);
    assert(reg.getComponent<Hitbox>(e).height == 6.F);
    assert(reg.getComponent<Health>(e).current == 7);
    assert(reg.getComponent<Health>(e).max == 10);
    assert(reg.getComponent<Lifetime>(e).remaining == 8.F);
    assert(reg.getComponent<PlayerOwner>(e).playerId == 3);
    assert(reg.getComponent<Projectile>(e).owner == e);
}

static void test_components_are_independent_per_entity()
{
    Registry reg;
    const Entity a = reg.createEntity();
    const Entity b = reg.createEntity();
    reg.addComponent(a, Position{1.F, 1.F});
    reg.addComponent(b, Position{2.F, 2.F});
    reg.getComponent<Position>(a).x = 9.F;
    assert(reg.getComponent<Position>(b).x == 2.F);
}

static void test_motion_moves_entities_with_velocity()
{
    Registry reg;
    const Entity moving = reg.createEntity();
    reg.addComponent(moving, Position{0.F, 0.F});
    reg.addComponent(moving, Velocity{2.F, -4.F});
    const Entity still = reg.createEntity();
    reg.addComponent(still, Position{5.F, 5.F});

    MotionSystem motion;
    motion.update(reg, 0.5F);

    assert(reg.getComponent<Position>(moving).x == 1.F);
    assert(reg.getComponent<Position>(moving).y == -2.F);
    assert(reg.getComponent<Position>(still).x == 5.F); // no Velocity: untouched
    assert(reg.getComponent<Position>(still).y == 5.F);
}

static void test_input_sets_velocity_from_keys()
{
    Registry reg;
    const Entity e = reg.createEntity();
    Controllable keys;
    keys.up = true;
    keys.right = true;
    reg.addComponent(e, keys);
    reg.addComponent(e, Velocity{});

    InputSystem input(100.F);
    input.update(reg, 0.1F);
    assert(reg.getComponent<Velocity>(e).dx == 100.F);
    assert(reg.getComponent<Velocity>(e).dy == -100.F);

    reg.getComponent<Controllable>(e) = Controllable{}; // all keys released
    input.update(reg, 0.1F);
    assert(reg.getComponent<Velocity>(e).dx == 0.F);
    assert(reg.getComponent<Velocity>(e).dy == 0.F);
}

static void test_input_opposite_keys_cancel()
{
    Registry reg;
    const Entity e = reg.createEntity();
    Controllable keys;
    keys.left = true;
    keys.right = true;
    reg.addComponent(e, keys);
    reg.addComponent(e, Velocity{});

    InputSystem input(100.F);
    input.update(reg, 0.1F);
    assert(reg.getComponent<Velocity>(e).dx == 0.F);
}

static void test_bounds_clamps_controllable_entities()
{
    Registry reg;
    const Entity e = reg.createEntity();
    reg.addComponent(e, Position{500.F, -20.F});
    reg.addComponent(e, Controllable{});

    BoundsSystem bounds(BoundsArea{0.F, 0.F, 100.F, 50.F}, 10.F);
    bounds.update(reg, 0.1F);

    assert(reg.isAlive(e));
    assert(reg.getComponent<Position>(e).x == 100.F);
    assert(reg.getComponent<Position>(e).y == 0.F);
}

static void test_bounds_destroys_entities_far_outside()
{
    Registry reg;
    const Entity inside = reg.createEntity();
    reg.addComponent(inside, Position{50.F, 25.F});
    const Entity inMargin = reg.createEntity();
    reg.addComponent(inMargin, Position{105.F, 25.F}); // outside but within the margin
    const Entity gone = reg.createEntity();
    reg.addComponent(gone, Position{200.F, 25.F});

    BoundsSystem bounds(BoundsArea{0.F, 0.F, 100.F, 50.F}, 10.F);
    bounds.update(reg, 0.1F);
    assert(reg.isAlive(gone)); // destruction is deferred
    reg.flushDestroyed();

    assert(reg.isAlive(inside));
    assert(reg.isAlive(inMargin));
    assert(!reg.isAlive(gone));
}

int main()
{
    test_defaults();
    test_components_can_be_stored_in_registry();
    test_components_are_independent_per_entity();
    std::cout << "engine components: all tests passed\n";
    test_motion_moves_entities_with_velocity();
    test_input_sets_velocity_from_keys();
    test_input_opposite_keys_cancel();
    test_bounds_clamps_controllable_entities();
    test_bounds_destroys_entities_far_outside();
    std::cout << "engine systems: all tests passed\n";
    return 0;
}
