#undef NDEBUG
#include "EntityManager.hpp"
#include "Registry.hpp"
#include "SparseSet.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace rtype::engine;

namespace {

struct Position
{
    float x;
    float y;
};

struct Velocity
{
    float dx;
    float dy;
};

struct Health
{
    int hp;
};

struct Hitbox
{
    float w;
    float h;
};

} // namespace

static void test_entity_default_is_null()
{
    Entity e;
    assert(e.isNull());
    assert(!Entity(0, 0).isNull());
}

static void test_entity_manager_create_destroy()
{
    EntityManager em;
    const Entity a = em.create();
    const Entity b = em.create();
    assert(a != b);
    assert(em.isAlive(a) && em.isAlive(b));
    assert(em.aliveCount() == 2);

    assert(em.destroy(a));
    assert(!em.isAlive(a));
    assert(em.aliveCount() == 1);
    assert(!em.destroy(a));
}

static void test_entity_manager_recycling_and_stale_id()
{
    EntityManager em;
    const Entity a = em.create();
    em.destroy(a);
    const Entity b = em.create();
    assert(b.index() == a.index());
    assert(b.generation() == a.generation() + 1);
    assert(!em.isAlive(a));
    assert(em.isAlive(b));
}

static void test_entity_manager_unknown_entities()
{
    EntityManager em;
    assert(!em.isAlive(Entity{}));
    assert(!em.isAlive(Entity(42, 0)));
    assert(!em.destroy(Entity{}));
}

static void test_sparse_set_add_get_has()
{
    SparseSet<int> set;
    const Entity e(3, 0);
    set.add(e, 42);
    assert(set.has(e));
    assert(set.get(e) == 42);
    assert(set.size() == 1);

    set.add(e, 7);
    assert(set.size() == 1);
    assert(set.get(e) == 7);
}

static void test_sparse_set_remove_keeps_dense_array_packed()
{
    SparseSet<int> set;
    const Entity a(0, 0), b(1, 0), c(2, 0);
    set.add(a, 10);
    set.add(b, 20);
    set.add(c, 30);

    set.remove(a);
    assert(!set.has(a));
    assert(set.size() == 2);
    assert(set.components().size() == 2);
    assert(set.get(b) == 20);
    assert(set.get(c) == 30);

    set.remove(Entity(9, 0));
    assert(set.size() == 2);
}

static void test_sparse_set_stale_generation_rejected()
{
    SparseSet<int> set;
    set.add(Entity(4, 0), 7);
    assert(!set.has(Entity(4, 1)));
}

static void test_sparse_set_null_entity_throws()
{
    SparseSet<int> set;
    bool thrown = false;
    try {
        set.add(Entity{}, 1);
    } catch (const std::invalid_argument&) {
        thrown = true;
    }
    assert(thrown);
}

static void test_sparse_set_get_missing_throws()
{
    SparseSet<int> set;
    bool thrown = false;
    try {
        set.get(Entity(1, 0));
    } catch (const std::out_of_range&) {
        thrown = true;
    }
    assert(thrown);
}

static void test_sparse_set_dense_iteration()
{
    SparseSet<int> set;
    for (std::uint32_t i = 0; i < 100; ++i) {
        set.add(Entity(i, 0), static_cast<int>(i));
    }
    int sum = 0;
    for (int v : set.components()) {
        sum += v;
    }
    assert(sum == 99 * 100 / 2);
}

static void test_sparse_set_through_base_interface()
{
    SparseSet<int> set;
    const Entity e(2, 0);
    set.add(e, 1);
    IComponentStorage& base = set;
    assert(base.has(e));
    base.remove(e);
    assert(base.size() == 0);
}

static void test_registry_components()
{
    Registry reg;
    const Entity e = reg.createEntity();
    reg.addComponent(e, Position{1.F, 2.F});
    reg.addComponent(e, Velocity{3.F, 4.F});

    assert(reg.hasComponent<Position>(e));
    assert(reg.getComponent<Velocity>(e).dx == 3.F);

    reg.getComponent<Position>(e).x = 9.F;
    assert(reg.getComponent<Position>(e).x == 9.F);

    reg.removeComponent<Velocity>(e);
    assert(!reg.hasComponent<Velocity>(e));
    assert(reg.hasComponent<Position>(e));
}

static void test_registry_destroy_cleans_every_storage()
{
    Registry reg;
    const Entity e = reg.createEntity();
    reg.addComponent(e, Position{1.F, 2.F});
    reg.addComponent(e, Velocity{3.F, 4.F});

    assert(reg.destroyEntity(e));
    assert(!reg.isAlive(e));
    assert(!reg.hasComponent<Position>(e));
    assert(!reg.hasComponent<Velocity>(e));
    assert(reg.getStorage<Position>().size() == 0);
    assert(reg.getStorage<Velocity>().size() == 0);
    assert(!reg.destroyEntity(e));
}

static void test_registry_recycled_entity_has_no_old_components()
{
    Registry reg;
    const Entity a = reg.createEntity();
    reg.addComponent(a, Position{1.F, 2.F});
    reg.destroyEntity(a);

    const Entity b = reg.createEntity();
    assert(!reg.hasComponent<Position>(b));
}

static void test_registry_missing_component_throws()
{
    Registry reg;
    const Entity e = reg.createEntity();
    bool thrown = false;
    try {
        reg.getComponent<Position>(e);
    } catch (const std::out_of_range&) {
        thrown = true;
    }
    assert(thrown);
}

static void test_registry_add_to_dead_entity_throws()
{
    Registry reg;
    const Entity e = reg.createEntity();
    reg.destroyEntity(e);
    bool thrown = false;
    try {
        reg.addComponent(e, Position{1.F, 2.F});
    } catch (const std::invalid_argument&) {
        thrown = true;
    }
    assert(thrown);
}

static void test_registry_const_lookup_creates_nothing()
{
    Registry reg;
    const Entity e = reg.createEntity();
    reg.addComponent(e, Position{1.F, 2.F});
    const Registry& creg = reg;
    assert(creg.hasComponent<Position>(e));
    assert(!creg.hasComponent<Velocity>(e));
    assert(creg.getComponent<Position>(e).x == 1.F);
}

static void test_each_one_component()
{
    Registry reg;
    for (int i = 0; i < 3; ++i) {
        reg.addComponent(reg.createEntity(), Position{1.F, 0.F});
    }
    int count = 0;
    reg.each<Position>([&](Entity, Position&) { ++count; });
    assert(count == 3);
}

static void test_each_only_entities_with_all_components()
{
    Registry reg;
    const Entity a = reg.createEntity();
    const Entity b = reg.createEntity();
    const Entity c = reg.createEntity();
    reg.addComponent(a, Position{0.F, 0.F});
    reg.addComponent(a, Velocity{1.F, 0.F});
    reg.addComponent(b, Position{0.F, 0.F});
    reg.addComponent(c, Velocity{1.F, 0.F});

    int count = 0;
    reg.each<Position, Velocity>([&](Entity e, Position& p, Velocity& v) {
        assert(e == a);
        p.x += v.dx;
        ++count;
    });
    assert(count == 1);
    assert(reg.getComponent<Position>(a).x == 1.F);
}

static void test_each_three_and_four_components()
{
    Registry reg;
    const Entity full = reg.createEntity();
    const Entity partial = reg.createEntity();
    reg.addComponent(full, Position{0.F, 0.F});
    reg.addComponent(full, Velocity{0.F, 0.F});
    reg.addComponent(full, Health{10});
    reg.addComponent(full, Hitbox{1.F, 1.F});
    reg.addComponent(partial, Position{0.F, 0.F});
    reg.addComponent(partial, Velocity{0.F, 0.F});
    reg.addComponent(partial, Health{10});

    int three = 0;
    reg.each<Position, Velocity, Health>([&](Entity, Position&, Velocity&, Health&) { ++three; });
    assert(three == 2);

    int four = 0;
    reg.each<Hitbox, Position, Velocity, Health>(
        [&](Entity e, Hitbox&, Position&, Velocity&, Health&) {
            assert(e == full);
            ++four;
        });
    assert(four == 1);
}

static void test_destroy_during_iteration_is_deferred()
{
    Registry reg;
    for (int i = 0; i < 5; ++i) {
        reg.addComponent(reg.createEntity(), Position{0.F, 0.F});
    }
    int visited = 0;
    reg.each<Position>([&](Entity e, Position&) {
        ++visited;
        reg.destroyLater(e);
    });
    assert(visited == 5);
    assert(reg.aliveCount() == 5);

    reg.flushDestroyed();
    assert(reg.aliveCount() == 0);
    assert(reg.getStorage<Position>().size() == 0);
}

static void test_flush_ignores_duplicates()
{
    Registry reg;
    const Entity e = reg.createEntity();
    reg.destroyLater(e);
    reg.destroyLater(e);
    reg.flushDestroyed();
    assert(!reg.isAlive(e));
    assert(reg.aliveCount() == 0);
}

int main()
{
    test_entity_default_is_null();
    test_entity_manager_create_destroy();
    test_entity_manager_recycling_and_stale_id();
    test_entity_manager_unknown_entities();

    test_sparse_set_add_get_has();
    test_sparse_set_remove_keeps_dense_array_packed();
    test_sparse_set_stale_generation_rejected();
    test_sparse_set_null_entity_throws();
    test_sparse_set_get_missing_throws();
    test_sparse_set_dense_iteration();
    test_sparse_set_through_base_interface();

    test_registry_components();
    test_registry_destroy_cleans_every_storage();
    test_registry_recycled_entity_has_no_old_components();
    test_registry_missing_component_throws();
    test_registry_add_to_dead_entity_throws();
    test_registry_const_lookup_creates_nothing();

    test_each_one_component();
    test_each_only_entities_with_all_components();
    test_each_three_and_four_components();

    test_destroy_during_iteration_is_deferred();
    test_flush_ignores_duplicates();

    std::cout << "engine ecs: all tests passed\n";
    return 0;
}
