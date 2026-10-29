#pragma once

#include "ClientComponents.hpp"
#include "GuiTypes.hpp"
#include "components/Components.hpp"
#include "ecs/Registry.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>

namespace rtype::client {

using PlayerNames = std::unordered_map<std::uint32_t, std::string>;

inline void addWorldSprites(gui::RenderScene& scene, engine::Registry& registry,
                            const PlayerNames& names)
{
    registry.each<Sprite, engine::Position, NetworkId>([&](engine::Entity, Sprite& sprite,
                                                           engine::Position& position,
                                                           NetworkId& net) {
        gui::SpriteDraw draw;
        draw.entityId = {net.id, 0};
        draw.textureId = sprite.textureId;
        draw.sourceRect = {{sprite.source.x, sprite.source.y}, sprite.source.w, sprite.source.h};
        draw.destinationRect = {{position.x - sprite.width / 2.F, position.y - sprite.height / 2.F},
                                sprite.width,
                                sprite.height};
        draw.layer = sprite.layer;
        scene.sprites.push_back(std::move(draw));
    });

    registry.each<PlayerSlot, engine::Position>(
        [&](engine::Entity, PlayerSlot& slot, engine::Position& position) {
            const auto it = names.find(static_cast<std::uint32_t>(slot.index) + 1);
            gui::TextDraw text;
            text.text = it != names.end() ? it->second : "Player";
            text.position = {position.x - 48.F, position.y - 45.F};
            scene.sceneTexts.push_back(std::move(text));
        });
}
} // namespace rtype::client
