#pragma once

#include "ISystem.hpp"
#include "components/Components.hpp"
#include "ecs/Entity.hpp"

#include <functional>

namespace rtype::engine {

using SpawnProjectileFn = std::function<void(Registry&, Entity, Position, unsigned)>;

class WeaponSystem : public ISystem
{
public:
    explicit WeaponSystem(SpawnProjectileFn spawn);
    void update(Registry& registry, float dt) override;

private:
    SpawnProjectileFn _spawn;
};

} // namespace rtype::engine
