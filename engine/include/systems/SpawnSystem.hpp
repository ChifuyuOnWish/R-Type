#pragma once

#include "ISystem.hpp"

#include <functional>
#include <random>

namespace rtype::engine {

using SpawnEnemyFn = std::function<void(Registry&, float, float)>;

struct SpawnConfig
{
    float interval = 1.F;
    float x = 0.F;
    float minY = 0.F;
    float maxY = 0.F;
};

class SpawnSystem : public ISystem
{
public:
    SpawnSystem(SpawnConfig config, unsigned seed, SpawnEnemyFn spawn);
    void update(Registry& registry, float dt) override;

private:
    SpawnConfig _config;
    SpawnEnemyFn _spawn;
    std::mt19937 _random;
    float _timer = 0.F;
};

} // namespace rtype::engine