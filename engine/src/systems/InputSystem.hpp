#pragma once

#include "ISystem.hpp"

namespace rtype::engine {

class InputSystem : public ISystem
{
public:
    explicit InputSystem(float speed);
    void update(Registry& registry, float dt) override;

private:
    float _speed;
};

} // namespace rtype::engine
