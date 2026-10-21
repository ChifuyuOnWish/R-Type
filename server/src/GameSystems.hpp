#pragma once

#include "ISystem.hpp"
#include "components/Components.hpp"
#include "ecs/EntityFactory.hpp"

#include <functional>
#include <random>

namespace rtype::server {

inline constexpr float WorldWidth = 1920.F;
inline constexpr float WorldHeight = 1080.F;

class WaveSystem : public engine::ISystem
{
public:
    using OnSpawn = std::function<void(engine::Entity)>;

    WaveSystem(const engine::EntityFactory& factory, OnSpawn onSpawn);
    void update(engine::Registry& registry, float dt) override;

private:
    const engine::EntityFactory& _factory;
    OnSpawn _onSpawn;
    float _timer = 0.F;
    std::mt19937 _rng{std::random_device{}()};
};

class RespawnSystem : public engine::ISystem
{
public:
    void update(engine::Registry& registry, float dt) override;
};

} // namespace rtype::server
