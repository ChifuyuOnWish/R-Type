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

// Player 1 keeps the drawing's own colours; the others recolour the same plane.
inline constexpr gui::GuiColor PlayerTints[] = {
    {255, 255, 255, 255}, // player 1: unused, original colours
    {226, 72, 60, 255},   // player 2: red
    {64, 176, 92, 255},   // player 3: green
    {236, 184, 40, 255},  // player 4: yellow
};

inline void addWorldSprites(gui::RenderScene& scene, engine::Registry& registry,
                            const PlayerNames& names, float animationSeconds = 0.0F)
{
    registry.each<Sprite, engine::Position, NetworkId, Extrapolation, engine::Velocity>(
        [&](engine::Entity entity, Sprite& sprite, engine::Position& position, NetworkId& net,
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
            draw.flipY = sprite.flipY;
            const auto phase = animationSeconds + static_cast<float>(net.id % 8) * 0.03F;
            const auto wave = std::sin(phase * 2.0F * std::numbers::pi_v<float>);
            switch (sprite.motion) {
            case SpriteMotion::Plane: {
                draw.ink = gui::SpriteInk::Original;
                draw.rotation = std::clamp(velocity.dy * 0.025F, -8.0F, 8.0F) + 1.2F * wave;
                if (registry.hasComponent<PlayerSlot>(entity)) {
                    const std::size_t slot = registry.getComponent<PlayerSlot>(entity).index;
                    if (slot > 0 && slot < std::size(PlayerTints)) {
                        draw.ink = gui::SpriteInk::TintedPencil;
                        draw.tint = PlayerTints[slot];
                    }
                }
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
            case SpriteMotion::Slug:
                draw.ink = gui::SpriteInk::BoldGraphite;
                if (sprite.shotPoseRemaining > 0.0F)
                    draw.sourceRect.pos.y += sprite.source.h;
                break;
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

inline void addGroundBands(gui::RenderScene& scene, float animationSeconds)
{
    constexpr float WorldWidth = 1920.F;
    constexpr float WorldHeight = 1080.F;
    constexpr float BandHeight = 60.F;
    constexpr float ScrollSpeed = 120.F;
    constexpr float Cell = 128.F;
    constexpr int BandLayer = 30;

    const float offset = std::fmod(animationSeconds * ScrollSpeed, BandHeight);
    for (float left = -offset; left < WorldWidth; left += BandHeight) {
        for (const bool ceiling : {false, true}) {
            gui::SpriteDraw draw;
            draw.textureId = "ground";
            draw.sourceRect = {{0.F, 0.F}, Cell, Cell};
            const float centerY = ceiling ? BandHeight / 2.F : WorldHeight - BandHeight / 2.F;
            draw.destinationRect = {{left + BandHeight / 2.F, centerY}, BandHeight, BandHeight};
            draw.origin = {BandHeight / 2.F, BandHeight / 2.F};
            draw.layer = BandLayer;
            draw.flipY = ceiling;
            scene.sprites.push_back(std::move(draw));
        }
    }
}

} // namespace rtype::client
