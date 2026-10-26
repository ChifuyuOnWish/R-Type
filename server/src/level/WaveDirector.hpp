#pragma once

#include "GameContext.hpp"
#include "LevelData.hpp"
#include "ecs/Entity.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <random>
#include <vector>

namespace rtype::server::level {

class WaveDirector
{
public:
    WaveDirector(const Level& level, const Scaling& scaling, std::uint32_t seed);

    void update(GameContext& context, float dt);
    bool finished() const;

private:
    struct Pending
    {
        float at;
        std::reference_wrapper<const EnemyDef> enemy;
        float y;
        int health;
    };

    void schedule(float previousStart);
    bool cleared() const;
    void startWave(const Wave& wave, float at, std::size_t players);
    void prune(engine::Registry& registry);

    const Level& _level;
    Scaling _scaling;
    std::mt19937 _rng;
    float _clock = 0.F;
    std::size_t _next = 0;
    std::optional<float> _nextWaveAt;
    std::vector<Pending> _pending;
    std::vector<engine::Entity> _spawned;
};

} // namespace rtype::server::level
