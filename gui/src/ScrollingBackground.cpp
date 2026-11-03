#include "ScrollingBackground.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>

namespace rtype::gui {
namespace {

constexpr double tileSize = 512.0;
constexpr std::uint32_t starsPerTile = 64;

std::uint32_t mix(std::uint32_t value)
{
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    return value ^ (value >> 16);
}

} // namespace

void ScrollingBackground::update(const BackgroundConfig& config, float elapsedSeconds)
{
    if (!std::isfinite(config.scrollSpeed) || config.scrollSpeed < 0.0F ||
        !std::isfinite(elapsedSeconds) || elapsedSeconds < 0.0F)
        throw std::invalid_argument(
            "Background speed and elapsed time must be finite and non-negative");
    if (!config.enabled)
        return;

    const double distance = static_cast<double>(config.scrollSpeed) * elapsedSeconds;
    _offset = std::fmod(_offset + distance, tileSize);
    _paperDistance += distance;
}

std::vector<SpriteDraw> ScrollingBackground::getTiles(GuiVector2 viewport, GuiVector2 textureSize,
                                                      const std::string& textureId) const
{
    if (!std::isfinite(viewport.x) || !std::isfinite(viewport.y) || !std::isfinite(textureSize.x) ||
        !std::isfinite(textureSize.y) || textureSize.x <= 0 || textureSize.y <= 0)
        throw std::invalid_argument("Paper background dimensions must be finite and positive");
    if (viewport.x <= 0 || viewport.y <= 0)
        return {};
    const double width = static_cast<double>(textureSize.x) * viewport.y / textureSize.y;
    if (!std::isfinite(width) || width > std::numeric_limits<float>::max())
        throw std::invalid_argument("Paper background tile is too wide");
    const double phase = std::fmod(_paperDistance, width);
    std::vector<SpriteDraw> result;
    for (double x = -phase; x < viewport.x || result.size() < 2; x += width) {
        SpriteDraw tile;
        tile.textureId = textureId;
        tile.sourceRect = {{0, 0}, textureSize.x, textureSize.y};
        tile.destinationRect = {{static_cast<float>(x), 0}, static_cast<float>(width), viewport.y};
        tile.layer = std::numeric_limits<int>::lowest();
        result.push_back(std::move(tile));
    }
    return result;
}

std::vector<GuiPosition> ScrollingBackground::getStars(GuiVector2 viewport) const
{
    if (!std::isfinite(viewport.x) || !std::isfinite(viewport.y))
        throw std::invalid_argument("Background viewport must be finite");
    std::vector<GuiPosition> result;
    if (viewport.x <= 0.0F || viewport.y <= 0.0F)
        return result;

    for (double tileY = 0.0; tileY < viewport.y; tileY += tileSize) {
        for (double tileX = -_offset; tileX < viewport.x; tileX += tileSize) {
            for (std::uint32_t index = 0; index < starsPerTile; ++index) {
                const double x = tileX + (index % 8) * 64 + 2 + mix(index * 2 + 1) % 60;
                const double y = tileY + (index / 8) * 64 + 2 + mix(index * 2 + 2) % 60;
                if (x + 2.0 > 0.0 && x < viewport.x && y < viewport.y)
                    result.push_back({static_cast<float>(x), static_cast<float>(y)});
            }
        }
    }
    return result;
}

} // namespace rtype::gui
