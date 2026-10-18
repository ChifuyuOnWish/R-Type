#pragma once

#include "ISystem.hpp"

namespace rtype::engine {

class DeathSystem : public ISystem
{
public:
    void update(Registry& registry, float dt) override;
};

} // namespace rtype::engine
