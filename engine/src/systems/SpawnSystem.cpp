#include "systems/SpawnSystem.hpp"

#include <utility>

namespace rtype::engine {

namespace {
float randomInRange(std::mt19937& random, float low, float high)
{
    const float ratio = static_cast<float>(random()) / static_cast<float>(random.max());
    return low + ratio * (high - low);
}
} // namespace

SpawnSystem::SpawnSystem(SpawnConfig config, unsigned seed, SpawnEnemyFn spawn)
    : _config(config), _spawn(std::move(spawn)), _random(seed), _timer(config.interval)
{
}

void SpawnSystem::update(Registry& registry, float dt)
{
    if (!_spawn || _config.interval <= 0.F) {
        return;
    }
    _timer -= dt;
    while (_timer <= 0.F) {
        _timer += _config.interval;
        _spawn(registry, _config.x, randomInRange(_random, _config.minY, _config.maxY));
    }
}

} // namespace rtype::engine
