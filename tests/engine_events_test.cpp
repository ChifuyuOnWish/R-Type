#undef NDEBUG
#include "EventDispatcher.hpp"
#include "Events.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace rtype::engine;

static void test_publish_is_queued_until_flush()
{
    EventDispatcher bus;
    int calls = 0;
    bus.subscribe<EntityDiedEvent>([&](const EntityDiedEvent&) { calls++; });
    bus.publish(EntityDiedEvent{});
    assert(calls == 0);
    bus.flush();
    assert(calls == 1);
    bus.flush();
    assert(calls == 1);
}

static void test_subscriber_receives_event_data()
{
    EventDispatcher bus;
    int received = 0;
    bus.subscribe<DamageEvent>([&](const DamageEvent& e) { received = e.amount; });
    bus.publish(DamageEvent{Entity{}, 7, Entity{}});
    bus.flush();
    assert(received == 7);
}

static void test_only_matching_type_is_delivered()
{
    EventDispatcher bus;
    int died = 0;
    int spawned = 0;
    bus.subscribe<EntityDiedEvent>([&](const EntityDiedEvent&) { died++; });
    bus.subscribe<EntitySpawnedEvent>([&](const EntitySpawnedEvent&) { spawned++; });
    bus.publish(EntityDiedEvent{});
    bus.flush();
    assert(died == 1 && spawned == 0);
}

static void test_events_keep_publication_order()
{
    EventDispatcher bus;
    std::vector<int> amounts;
    bus.subscribe<DamageEvent>([&](const DamageEvent& e) { amounts.push_back(e.amount); });
    bus.publish(DamageEvent{Entity{}, 1, Entity{}});
    bus.publish(DamageEvent{Entity{}, 2, Entity{}});
    bus.publish(DamageEvent{Entity{}, 3, Entity{}});
    bus.flush();
    assert((amounts == std::vector<int>{1, 2, 3}));
}

static void test_several_subscribers()
{
    EventDispatcher bus;
    int a = 0;
    int b = 0;
    bus.subscribe<CollisionEvent>([&](const CollisionEvent&) { a++; });
    bus.subscribe<CollisionEvent>([&](const CollisionEvent&) { b++; });
    bus.publish(CollisionEvent{});
    bus.flush();
    assert(a == 1 && b == 1);
}

static void test_unsubscribe_stops_delivery()
{
    EventDispatcher bus;
    int calls = 0;
    const SubscriptionId id =
        bus.subscribe<EntityDiedEvent>([&](const EntityDiedEvent&) { calls++; });
    bus.unsubscribe(id);
    bus.unsubscribe(id);
    bus.publish(EntityDiedEvent{});
    bus.flush();
    assert(calls == 0);
}

static void test_unsubscribe_during_delivery_is_safe()
{
    EventDispatcher bus;
    int calls = 0;
    SubscriptionId id = 0;
    id = bus.subscribe<EntityDiedEvent>([&](const EntityDiedEvent&) {
        calls++;
        bus.unsubscribe(id);
    });
    bus.publish(EntityDiedEvent{});
    bus.publish(EntityDiedEvent{});
    bus.flush();
    assert(calls == 1);
}

static void test_publish_without_subscriber_is_harmless()
{
    EventDispatcher bus;
    bus.publish(EntitySpawnedEvent{});
    bus.flush();
}

static void test_two_systems_communicate_without_referencing_each_other()
{
    EventDispatcher bus;
    int damageTaken = 0;
    bus.subscribe<CollisionEvent>(
        [&](const CollisionEvent& e) { bus.publish(DamageEvent{e.b, 5, Entity{}}); });
    bus.subscribe<DamageEvent>([&](const DamageEvent& e) { damageTaken += e.amount; });
    bus.publish(CollisionEvent{});
    bus.flush();
    assert(damageTaken == 5);
}

int main()
{
    test_publish_is_queued_until_flush();
    test_subscriber_receives_event_data();
    test_only_matching_type_is_delivered();
    test_events_keep_publication_order();
    test_several_subscribers();
    test_unsubscribe_stops_delivery();
    test_unsubscribe_during_delivery_is_safe();
    test_publish_without_subscriber_is_harmless();
    test_two_systems_communicate_without_referencing_each_other();
    std::cout << "engine events: all tests passed\n";
    return 0;
}