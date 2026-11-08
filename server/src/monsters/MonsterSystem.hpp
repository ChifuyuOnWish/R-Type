#pragma once

#include "ISystem.hpp"

#include <random>

namespace rtype::server {

class MonsterSystem : public engine::ISystem
{
public:
    explicit MonsterSystem(std::mt19937& rng);
    void update(engine::Registry& registry, float dt) override;

private:
    std::mt19937& _rng;
};

} // namespace rtype::server
