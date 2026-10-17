#undef NDEBUG
#include "GameLoop.hpp"
#include "Registry.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace rtype::engine;

namespace {

struct Marker
{
    int value;
};

class RecordingSystem : public ISystem
{
public:
    RecordingSystem(std::vector<std::string>& log, std::string name)
        : _log(log), _name(std::move(name))
    {
    }
    void update(Registry&, float) override { _log.push_back(_name); }

private:
    std::vector<std::string>& _log;
    std::string _name;
};

class CountingSystem : public ISystem
{
public:
    void update(Registry&, float) override { calls++; }
    int calls = 0;
};

class DestroySystem : public ISystem
{
public:
    void update(Registry& registry, float) override
    {
        registry.each<Marker>([&](Entity e, Marker&) { registry.destroyLater(e); });
    }
};

constexpr double Margin = 0.001; // keeps float rounding away from step boundaries

} // namespace

static void test_systems_run_in_order()
{
    std::vector<std::string> log;
    Registry reg;
    double time = 0.0;
    GameLoop loop(reg, [&] { return time; });
    loop.addSystem(std::make_unique<RecordingSystem>(log, "input"));
    loop.addSystem(std::make_unique<RecordingSystem>(log, "motion"));
    loop.addSystem(std::make_unique<RecordingSystem>(log, "collision"));

    time += GameLoop::Step + Margin;
    assert(loop.tick() == 1);
    assert((log == std::vector<std::string>{"input", "motion", "collision"}));
}

static void test_null_system_is_rejected()
{
    Registry reg;
    GameLoop loop(reg);
    bool thrown = false;
    try {
        loop.addSystem(nullptr);
    } catch (const std::invalid_argument&) {
        thrown = true;
    }
    assert(thrown);
}

static void test_fast_machine_waits_for_enough_time()
{
    Registry reg;
    double time = 0.0;
    GameLoop loop(reg, [&] { return time; });
    auto system = std::make_unique<CountingSystem>();
    CountingSystem* ptr = system.get();
    loop.addSystem(std::move(system));

    for (int i = 0; i < 3; i++) {
        time += GameLoop::Step / 4.0;
        assert(loop.tick() == 0);
    }
    time += GameLoop::Step / 4.0 + Margin;
    assert(loop.tick() == 1);
    assert(ptr->calls == 1);
}

static void test_slow_machine_catches_up()
{
    Registry reg;
    double time = 0.0;
    GameLoop loop(reg, [&] { return time; });
    auto system = std::make_unique<CountingSystem>();
    CountingSystem* ptr = system.get();
    loop.addSystem(std::move(system));

    time += 3.0 * GameLoop::Step + Margin;
    assert(loop.tick() == 3);
    assert(ptr->calls == 3);
}

static void test_catch_up_is_capped_and_lost_time_dropped()
{
    Registry reg;
    double time = 0.0;
    GameLoop loop(reg, [&] { return time; });
    auto system = std::make_unique<CountingSystem>();
    CountingSystem* ptr = system.get();
    loop.addSystem(std::move(system));

    time += 10.0; // 600 steps of lag
    assert(loop.tick() == GameLoop::MaxCatchUpSteps);
    assert(ptr->calls == static_cast<int>(GameLoop::MaxCatchUpSteps));
    assert(loop.tick() == 0); // the remaining debt was dropped
}

static void test_first_tick_ignores_time_before_construction()
{
    Registry reg;
    double time = 100.0;
    GameLoop loop(reg, [&] { return time; });
    assert(loop.tick() == 0);
}

static void test_deferred_destruction_is_flushed_each_tick()
{
    Registry reg;
    const Entity e = reg.createEntity();
    reg.addComponent(e, Marker{1});

    double time = 0.0;
    GameLoop loop(reg, [&] { return time; });
    loop.addSystem(std::make_unique<DestroySystem>());

    time += GameLoop::Step + Margin;
    assert(loop.tick() == 1);
    assert(!reg.isAlive(e));
}

static void test_server_runs_without_any_client_system()
{
    // The server only adds its own systems; the engine needs nothing else.
    Registry reg;
    double time = 0.0;
    GameLoop loop(reg, [&] { return time; });
    auto motion = std::make_unique<CountingSystem>();
    CountingSystem* ptr = motion.get();
    loop.addSystem(std::move(motion));

    time += GameLoop::Step + Margin;
    loop.tick();
    assert(ptr->calls == 1);
}

int main()
{
    test_systems_run_in_order();
    test_null_system_is_rejected();
    test_fast_machine_waits_for_enough_time();
    test_slow_machine_catches_up();
    test_catch_up_is_capped_and_lost_time_dropped();
    test_first_tick_ignores_time_before_construction();
    test_deferred_destruction_is_flushed_each_tick();
    test_server_runs_without_any_client_system();
    std::cout << "engine game loop: all tests passed\n";
    return 0;
}