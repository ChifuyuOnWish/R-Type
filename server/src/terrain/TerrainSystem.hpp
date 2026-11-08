#pragma once

#include "ISystem.hpp"

namespace rtype::server {

class TerrainSystem : public engine::ISystem
{
public:
    void update(engine::Registry& registry, float dt) override;
};

} // namespace rtype::server
