#pragma once

#include "ISystem.hpp"
#include "Registry.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace rtype::engine {

using ClockFn = std::function<double()>;

class GameLoop
{
public:
    static constexpr double Step = 1.0 / 60.0;
    static constexpr unsigned MaxCatchUpSteps = 5;

    explicit GameLoop(Registry& registry, ClockFn clock = ClockFn());
    void addSystem(std::unique_ptr<ISystem> system);
    unsigned tick();

private:
    Registry& _registry;
    ClockFn _clock;
    std::vector<std::unique_ptr<ISystem>> _systems;
    double _lastTime;
    double _accumulator = 0.0;
};

} // namespace rtype::engine
