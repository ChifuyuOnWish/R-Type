#pragma once

#include "ClientComponents.hpp"
#include "GuiTypes.hpp"
#include "components/Components.hpp"
#include "ecs/Registry.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string>
#include <unordered_map>
#include <utility>

namespace rtype::client {

using PlayerNames = std::unordered_map<std::uint32_t, std::string>;

inline void addWorldSprites(gui::RenderScene& scene, engine::Registry& registry,
                            const PlayerNames& names, float animationSeconds = 0.0F)
{
    registry.each<Sprite, engine::Position, NetworkId, Extrapolation, engine::Velocity>(
        [&](engine::Entity, Sprite& sprite, engine::Position& position, NetworkId& net,
            Extrapolation& motion, engine::Velocity& velocity) {
            gui::SpriteDraw draw;
            draw.entityId = {net.id, 0};
            draw.textureId = sprite.textureId;
            draw.sourceRect = {
                {sprite.source.x, sprite.source.y}, sprite.source.w, sprite.source.h};
            const gui::GuiPosition center{position.x + motion.offsetX, position.y + motion.offsetY};
            draw.origin = {sprite.width / 2.0F, sprite.height / 2.0F};
            draw.destinationRect = {center, sprite.width, sprite.height};
            draw.layer = sprite.layer;
            const auto phase = animationSeconds + static_cast<float>(net.id % 8) * 0.03F;
            const auto wave = std::sin(phase * 2.0F * std::numbers::pi_v<float>);
            switch (sprite.motion) {
            case SpriteMotion::Plane: {
                draw.ink = gui::SpriteInk::ColoredPencil;
                draw.rotation = std::clamp(velocity.dy * 0.025F, -8.0F, 8.0F) + 1.2F * wave;
                break;
            }
            case SpriteMotion::Bat: {
                draw.ink = gui::SpriteInk::BoldGraphite;
                const auto wing = static_cast<std::size_t>(phase * (25.0F / 3.0F)) % 2;
                draw.sourceRect.pos.x += sprite.source.w * static_cast<float>(wing);
                if (sprite.shotPoseRemaining > 0.0F)
                    draw.sourceRect.pos.y += sprite.source.h;
                break;
            }
            case SpriteMotion::Hover:
                draw.rotation = 3.0F * wave;
                break;
            case SpriteMotion::AlliedFire:
                draw.rotation = 180.0F;
                draw.tint = {155, 215, 255, 255};
                break;
            case SpriteMotion::EnemyFire:
            case SpriteMotion::None:
                break;
            }
            scene.sprites.push_back(std::move(draw));
        });

    registry.each<PlayerSlot, engine::Position, Extrapolation, Sprite>(
        [&](engine::Entity, PlayerSlot& slot, engine::Position& position, Extrapolation& motion,
            Sprite& sprite) {
            const auto it = names.find(static_cast<std::uint32_t>(slot.index) + 1);
            gui::TextDraw text;
            text.text = it != names.end() ? it->second : "Player";
            text.color = {0, 0, 0, 255};
            text.position = {position.x + motion.offsetX - sprite.width / 4.F,
                             position.y + motion.offsetY - sprite.height / 2.F - 24.F};
            scene.sceneTexts.push_back(std::move(text));
        });
}
} // namespace rtype::client
