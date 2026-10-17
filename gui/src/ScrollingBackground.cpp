#include "ScrollingBackground.hpp"

#include <cmath>
#include <cstdint>
#include <stdexcept>

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

    _offset =
        std::fmod(_offset + static_cast<double>(config.scrollSpeed) * elapsedSeconds, tileSize);
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
