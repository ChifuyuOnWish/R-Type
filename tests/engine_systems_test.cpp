#undef NDEBUG
#include "Components.hpp"
#include "Registry.hpp"
#include "systems/BoundsSystem.hpp"
#include "systems/CollisionSystem.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/DeathSystem.hpp"
#include "systems/EnemyBehaviorSystem.hpp"
#include "systems/InputSystem.hpp"
#include "systems/LifetimeSystem.hpp"
#include "systems/MotionSystem.hpp"
#include "systems/WeaponSystem.hpp"

#include <cassert>
#include <iostream>
#include <type_traits>

using namespace rtype::engine;

static_assert(std::is_trivially_copyable_v<Position>);
static_assert(std::is_trivially_copyable_v<Velocity>);
static_assert(std::is_trivially_copyable_v<Hitbox>);
static_assert(std::is_trivially_copyable_v<Health>);
static_assert(std::is_trivially_copyable_v<Damage>);
static_assert(std::is_trivially_copyable_v<Hit>);
static_assert(std::is_trivially_copyable_v<Lifetime>);
static_assert(std::is_trivially_copyable_v<Invincibility>);
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
    assert(Hit{}.amount == 0);
    assert(Lifetime{}.remaining == 0.F);
    assert(Invincibility{}.remaining == 0.F);
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
    assert(reg.getComponent<Position>(still).x == 5.F);
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

    reg.getComponent<Controllable>(e) = Controllable{};
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
    reg.addComponent(inMargin, Position{105.F, 25.F});
    const Entity gone = reg.createEntity();
    reg.addComponent(gone, Position{200.F, 25.F});

    BoundsSystem bounds(BoundsArea{0.F, 0.F, 100.F, 50.F}, 10.F);
    bounds.update(reg, 0.1F);
    assert(reg.isAlive(gone));
    reg.flushDestroyed();

    assert(reg.isAlive(inside));
    assert(reg.isAlive(inMargin));
    assert(!reg.isAlive(gone));
}

static Entity makeBox(Registry& reg, float x, float y, float w, float h)
{
    const Entity e = reg.createEntity();
    reg.addComponent(e, Position{x, y});
    reg.addComponent(e, Hitbox{w, h});
    return e;
}

static void test_overlap_gives_hit_to_the_one_with_health()
{
    Registry reg;
    const Entity shot = makeBox(reg, 0.F, 0.F, 4.F, 4.F);
    reg.addComponent(shot, Damage{2});
    const Entity enemy = makeBox(reg, 1.F, 0.F, 10.F, 10.F);
    reg.addComponent(enemy, Health{3, 3});

    CollisionSystem collision;
    collision.update(reg, 0.1F);
    assert(reg.hasComponent<Hit>(enemy));
    assert(reg.getComponent<Hit>(enemy).amount == 2);
    assert(!reg.hasComponent<Hit>(shot));
}

static void test_separate_and_touching_boxes_do_not_collide()
{
    Registry reg;
    const Entity shot = makeBox(reg, 0.F, 0.F, 10.F, 10.F);
    reg.addComponent(shot, Damage{1});
    const Entity far = makeBox(reg, 50.F, 0.F, 10.F, 10.F);
    reg.addComponent(far, Health{3, 3});
    const Entity touching = makeBox(reg, 10.F, 0.F, 10.F, 10.F);
    reg.addComponent(touching, Health{3, 3});

    CollisionSystem collision;
    collision.update(reg, 0.1F);
    assert(!reg.hasComponent<Hit>(far));
    assert(!reg.hasComponent<Hit>(touching));
}

static void test_two_shots_add_up()
{
    Registry reg;
    const Entity enemy = makeBox(reg, 0.F, 0.F, 10.F, 10.F);
    reg.addComponent(enemy, Health{10, 10});
    for (int i = 0; i < 2; i++) {
        const Entity shot = makeBox(reg, 1.F, 0.F, 2.F, 2.F);
        reg.addComponent(shot, Damage{3});
    }

    CollisionSystem collision;
    collision.update(reg, 0.1F);
    assert(reg.getComponent<Hit>(enemy).amount == 6);
}

static void test_hit_is_applied_then_removed()
{
    Registry reg;
    const Entity e = reg.createEntity();
    reg.addComponent(e, Health{5, 5});
    reg.addComponent(e, Hit{2});

    DamageSystem damage;
    damage.update(reg, 0.1F);
    assert(reg.getComponent<Health>(e).current == 3);
    assert(!reg.hasComponent<Hit>(e));
}

static void test_damage_is_clamped_at_zero()
{
    Registry reg;
    const Entity e = reg.createEntity();
    reg.addComponent(e, Health{3, 3});
    reg.addComponent(e, Hit{10});

    DamageSystem damage;
    damage.update(reg, 0.1F);
    assert(reg.getComponent<Health>(e).current == 0);
}

static void test_collision_then_damage_pipeline()
{
    Registry reg;
    const Entity shot = makeBox(reg, 0.F, 0.F, 4.F, 4.F);
    reg.addComponent(shot, Damage{2});
    const Entity enemy = makeBox(reg, 1.F, 0.F, 10.F, 10.F);
    reg.addComponent(enemy, Health{3, 3});

    CollisionSystem collision;
    DamageSystem damage;
    collision.update(reg, 0.1F);
    damage.update(reg, 0.1F);
    assert(reg.getComponent<Health>(enemy).current == 1);
    assert(!reg.hasComponent<Hit>(enemy));
}

static void test_lifetime_destroys_expired_entities()
{
    Registry reg;
    const Entity e = reg.createEntity();
    reg.addComponent(e, Lifetime{0.25F});

    LifetimeSystem lifetime;
    lifetime.update(reg, 0.1F);
    lifetime.update(reg, 0.1F);
    reg.flushDestroyed();
    assert(reg.isAlive(e));

    lifetime.update(reg, 0.1F);
    assert(reg.isAlive(e));
    reg.flushDestroyed();
    assert(!reg.isAlive(e));
}

static void test_death_destroys_entities_without_health()
{
    Registry reg;
    const Entity dead = reg.createEntity();
    reg.addComponent(dead, Health{0, 3});
    const Entity alive = reg.createEntity();
    reg.addComponent(alive, Health{1, 3});

    DeathSystem death;
    death.update(reg, 0.1F);
    reg.flushDestroyed();
    assert(!reg.isAlive(dead));
    assert(reg.isAlive(alive));
}

static void test_weapon_fires_with_cooldown()
{
    Registry reg;
    const Entity shooter = reg.createEntity();
    reg.addComponent(shooter, Position{10.F, 20.F});
    Controllable keys;
    keys.shoot = true;
    reg.addComponent(shooter, keys);
    reg.addComponent(shooter, Weapon{0.5F, 0.F, 7, 1.F});

    int shots = 0;
    Position lastOrigin;
    unsigned lastType = 0;
    WeaponSystem weapon([&](Registry& r, Entity owner, Position origin, unsigned type) {
        const Entity missile = r.createEntity();
        r.addComponent(missile, Position{origin.x, origin.y});
        r.addComponent(missile, Projectile{owner, 2.F, 0});
        lastOrigin = origin;
        lastType = type;
        shots++;
    });

    weapon.update(reg, 0.25F);
    assert(shots == 1);
    weapon.update(reg, 0.25F);
    assert(shots == 1);
    weapon.update(reg, 0.25F);
    assert(shots == 2);
    assert(lastOrigin.x == 10.F && lastOrigin.y == 20.F);
    assert(lastType == 7);
}

static void test_weapon_does_not_fire_without_shoot()
{
    Registry reg;
    const Entity shooter = reg.createEntity();
    reg.addComponent(shooter, Position{});
    reg.addComponent(shooter, Controllable{});
    reg.addComponent(shooter, Weapon{0.5F, 0.F, 0, 1.F});

    int shots = 0;
    WeaponSystem weapon([&](Registry&, Entity, Position, unsigned) { shots++; });
    weapon.update(reg, 0.25F);
    assert(shots == 0);
}

static void test_straight_enemy_moves_left()
{
    Registry reg;
    const Entity e = reg.createEntity();
    EnemyBehavior behavior;
    behavior.pattern = EnemyPattern::Straight;
    behavior.speed = 30.F;
    reg.addComponent(e, behavior);
    reg.addComponent(e, Velocity{});

    EnemyBehaviorSystem enemies;
    enemies.update(reg, 0.1F);
    assert(reg.getComponent<Velocity>(e).dx == -30.F);
    assert(reg.getComponent<Velocity>(e).dy == 0.F);
}

int main()
{
    test_defaults();
    test_components_can_be_stored_in_registry();
    test_components_are_independent_per_entity();

    test_motion_moves_entities_with_velocity();
    test_input_sets_velocity_from_keys();
    test_input_opposite_keys_cancel();
    test_bounds_clamps_controllable_entities();
    test_bounds_destroys_entities_far_outside();

    test_overlap_gives_hit_to_the_one_with_health();
    test_separate_and_touching_boxes_do_not_collide();
    test_two_shots_add_up();
    test_hit_is_applied_then_removed();
    test_damage_is_clamped_at_zero();
    test_collision_then_damage_pipeline();

    test_lifetime_destroys_expired_entities();
    test_death_destroys_entities_without_health();
    test_weapon_fires_with_cooldown();
    test_weapon_does_not_fire_without_shoot();

    test_straight_enemy_moves_left();

    std::cout << "engine: all tests passed\n";
    return 0;
}