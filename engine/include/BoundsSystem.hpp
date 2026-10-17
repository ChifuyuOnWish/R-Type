#pragma once

#include "ISystem.hpp"

namespace rtype::engine {

struct BoundsArea
{
    float minX = 0.F;
    float minY = 0.F;
    float maxX = 0.F;
    float maxY = 0.F;
};

class BoundsSystem : public ISystem
{
public:
    BoundsSystem(BoundsArea area, float margin);
    void update(Registry& registry, float dt) override;

private:
    BoundsArea _area;
    float _margin;
};

} // namespace rtype::engine
