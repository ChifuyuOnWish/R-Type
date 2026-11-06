#pragma once

#include "GuiTypes.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <utility>

namespace rtype::gui::detail {

inline GuiRectangle settingsButtonBounds(GuiVector2 viewport)
{
    const float scale = std::min(viewport.x / 1280.0F, viewport.y / 720.0F);
    const float side = std::max(36.0F, 64.0F * scale);
    return {{std::max(12.0F, viewport.x - side - 16.0F), 12.0F}, side, side};
}

inline GuiColor warmPaper(GuiColor tint)
{
    tint.green = static_cast<std::uint8_t>(static_cast<unsigned>(tint.green) * 247U / 255U);
    tint.blue = static_cast<std::uint8_t>(static_cast<unsigned>(tint.blue) * 232U / 255U);
    return tint;
}

inline bool menuArtwork(RenderScene& scene, const std::map<std::string, GuiVector2>& artwork,
                        const std::string& id, GuiRectangle bounds, GuiColor tint = {})
{
    const auto found = artwork.find(id);
    if (found == artwork.end())
        return false;
    const auto size = found->second;
    if (size.x <= 0 || size.y <= 0)
        return false;
    GuiRectangle source{{0, 0}, size.x, size.y};
    const float fit = std::min(bounds.width / source.width, bounds.height / source.height);
    bounds.pos.x += (bounds.width - source.width * fit) / 2;
    bounds.pos.y += (bounds.height - source.height * fit) / 2;
    bounds.width = source.width * fit;
    bounds.height = source.height * fit;
    SpriteDraw sprite;
    sprite.textureId = "ui-" + id;
    sprite.sourceRect = source;
    sprite.destinationRect = bounds;
    sprite.tint = tint;
    sprite.layer = id == "settings-background" || id == "background" ? -1000 : 0;
    scene.sprites.push_back(std::move(sprite));
    return true;
}

inline std::string paperArtwork(const std::map<std::string, GuiVector2>& artwork,
                                GuiRectangle bounds)
{
    const float ratio = bounds.width / bounds.height;
    float closest = 0;
    std::string result;
    for (const auto* name : {"paper-button", "paper-action", "paper-category", "paper-small",
                             "paper-mode", "paper-square"}) {
        const auto found = artwork.find(name);
        if (found == artwork.end() || found->second.x <= 0 || found->second.y <= 0)
            continue;
        const float candidate = found->second.x / found->second.y;
        const float fill = std::min(ratio / candidate, candidate / ratio);
        if (fill > closest) {
            closest = fill;
            result = name;
        }
    }
    return result;
}

} // namespace rtype::gui::detail
