#include "GameLoop.hpp"

#include <chrono>
#include <stdexcept>
#include <utility>

namespace rtype::engine {

namespace {

double steadyNow()
{
    const auto since = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<double>(since).count();
}

} // namespace

GameLoop::GameLoop(Registry& registry, ClockFn clock)
    : _registry(registry), _clock(clock ? std::move(clock) : ClockFn(steadyNow)),
      _lastTime(_clock())
{
}

void GameLoop::addSystem(std::unique_ptr<ISystem> system)
{
    if (!system) {
        throw std::invalid_argument("GameLoop::addSystem: null system");
    }
    _systems.push_back(std::move(system));
}

unsigned GameLoop::tick()
{
    const double current = _clock();
    const double frameTime = current - _lastTime;
    _lastTime = current;

    if (frameTime > 0.0) {
        _accumulator += frameTime;
    }
    unsigned steps = 0;
    while (_accumulator >= Step && steps < MaxCatchUpSteps) {
        for (auto& system : _systems) {
            system->update(_registry, static_cast<float>(Step));
        }
        _registry.flushDestroyed();
        _accumulator -= Step;
        steps++;
    }
    if (_accumulator >= Step) {
        _accumulator = 0.0;
    }
    return steps;
}

} // namespace rtype::engine
