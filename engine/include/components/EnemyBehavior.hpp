#pragma once

namespace rtype::engine {

enum class EnemyPattern
{
    Straight,
    Sine,
    Chase,
};

struct EnemyBehavior
{
    EnemyPattern pattern = EnemyPattern::Straight;
    float speed = 0.F;
    float amplitude = 0.F;
    float frequency = 0.F;
};

} // namespace rtype::engine
