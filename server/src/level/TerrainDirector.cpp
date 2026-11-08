#include "level/TerrainDirector.hpp"

#include "World.hpp"

#include <algorithm>
#include <iostream>

namespace rtype::server::level {

TerrainDirector::TerrainDirector(const Terrain& terrain) : _terrain(terrain)
{
    for (const std::string& row : _terrain.map)
        _columns = std::max(_columns, row.size());
}

bool TerrainDirector::finished() const
{
    return _next >= _columns && _tiles.empty();
}

void TerrainDirector::update(GameContext& context, float dt)
{
    _clock += dt;
    std::erase_if(_tiles, [&context](engine::Entity e) { return !context.registry.isAlive(e); });
    if (_clock < _terrain.startAt)
        return;

    const float scrolled = (_clock - _terrain.startAt) * _terrain.scrollSpeed;
    while (_next < _columns) {
        const float leftX = WorldWidth + static_cast<float>(_next) * _terrain.tileSize - scrolled;
        if (leftX > WorldWidth)
            break;
        spawnColumn(context, _next, leftX);
        ++_next;
        if (_next == _columns)
            std::cout << "terrain: last column in\n";
    }
}

void TerrainDirector::spawnColumn(GameContext& context, std::size_t column, float leftX)
{
    const float size = _terrain.tileSize;
    for (std::size_t row = 0; row < _terrain.map.size(); ++row) {
        const std::string& line = _terrain.map[row];
        if (column >= line.size() || line[column] == ' ')
            continue;
        const auto def = _terrain.tiles.find(line[column]);
        if (def == _terrain.tiles.end())
            continue;
        const float top = static_cast<float>(row) * size;
        if (top + size <= PlayTop || top >= PlayBottom)
            continue;
        _tiles.push_back(context.spawner.spawnTile(
            def->second, engine::Position{leftX + size / 2.F, top + size / 2.F}, size,
            _terrain.scrollSpeed));
    }
}

} // namespace rtype::server::level
