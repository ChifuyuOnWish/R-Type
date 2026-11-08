#pragma once

#include "GameContext.hpp"
#include "LevelData.hpp"
#include "ecs/Entity.hpp"

#include <cstddef>
#include <vector>

namespace rtype::server::level {

class TerrainDirector
{
public:
    explicit TerrainDirector(const Terrain& terrain);

    void update(GameContext& context, float dt);

    bool finished() const;

private:
    void spawnColumn(GameContext& context, std::size_t column, float leftX);

    const Terrain& _terrain;
    std::size_t _columns = 0;
    float _clock = 0.F;
    std::size_t _next = 0;
    std::vector<engine::Entity> _tiles;
};

} // namespace rtype::server::level
