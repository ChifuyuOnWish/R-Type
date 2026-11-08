#pragma once

#include "BossMessages.hpp"
#include "ClientComponents.hpp"
#include "GuiTypes.hpp"
#include "components/Components.hpp"
#include "ecs/Registry.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>

namespace rtype::client {

using PlayerNames = std::unordered_map<std::uint32_t, std::string>;
using BossState = std::optional<protocol::BossStateMessage>;

inline constexpr gui::GuiColor PlayerTints[] = {
    {255, 255, 255, 255}, // player 1: unused, original colours
    {226, 72, 60, 255},   // player 2: red
    {64, 176, 92, 255},   // player 3: green
    {236, 184, 40, 255},  // player 4: yellow
};

inline constexpr float DissolvePortion = 0.8F;

inline void addDeathParticles(gui::RenderScene& scene, gui::GuiPosition center, float width,
                              float height, std::uint32_t seed, float progress, gui::GuiColor color)
{
    constexpr int Count = 16;
    std::minstd_rand random(seed);
    std::uniform_real_distribution<float> unit(0.0F, 1.0F);
    for (int i = 0; i < Count; i++) {
        const float x = unit(random) - 0.5F;
        const float y = unit(random) - 0.5F;
        const float speed = 0.5F + unit(random);
        gui::ParticleDraw particle;
        particle.position = {center.x + x * width + 70.0F * speed * progress,
                             center.y + y * height - 40.0F * speed * progress};
        particle.size = 5.0F * (1.0F - progress);
        particle.color = color;
        particle.color.alpha =
            static_cast<std::uint8_t>(static_cast<float>(color.alpha) * (1.0F - progress));
        scene.particles.push_back(particle);
    }
}

inline void addWorldSprites(gui::RenderScene& scene, engine::Registry& registry,
                            const PlayerNames& names, float animationSeconds = 0.0F,
                            const BossState& boss = std::nullopt)
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
            case SpriteMotion::Boss: {
                draw.ink = gui::SpriteInk::BoldGraphite;
                draw.rotation = 1.5F * wave; // heavy, slower rock than the small tank's hover
                const bool known = boss && boss->entityId == net.id;
                if (known && boss->has(protocol::BossStateMessage::Dying)) {
                    const bool flash = static_cast<int>(animationSeconds * 12.0F) % 2 == 0;
                    draw.tint = flash ? gui::GuiColor{255, 120, 60, 255}
                                      : gui::GuiColor{255, 255, 255, 255};
                    draw.rotation = 6.0F * wave;
                } else if (known && boss->has(protocol::BossStateMessage::Enraged)) {
                    draw.tint = {255, 150, 130, 255};
                }
                break;
            }
            case SpriteMotion::EnemyFire:
            case SpriteMotion::None:
                break;
            }
            if (registry.hasComponent<Dying>(entity)) {
                const float t = std::clamp(
                    registry.getComponent<Dying>(entity).elapsed / DeathDuration, 0.0F, 1.0F);
                draw.dissolve = std::clamp(t / DissolvePortion, 0.0F, 1.0F);
                const gui::GuiColor dust = draw.ink == gui::SpriteInk::TintedPencil
                                               ? draw.tint
                                               : gui::GuiColor{70, 68, 72, 200};
                addDeathParticles(scene, center, sprite.width, sprite.height, net.id, t, dust);
            }
            if (sprite.hitFlashRemaining > 0.0F) {
                const float progress =
                    std::clamp(1.0F - sprite.hitFlashRemaining / HitFlashDuration, 0.0F, 1.0F);
                const float fade = 1.0F - progress;
                const float pulse = 0.5F + 0.5F * std::cos(progress * HitFlashBlinks * 2.0F *
                                                           std::numbers::pi_v<float>);
                draw.flash = HitFlashStrength * pulse * fade;
                const float scale = 1.0F + HitPopScale * fade * fade;
                draw.destinationRect.width = sprite.width * scale;
                draw.destinationRect.height = sprite.height * scale;
                draw.origin = {draw.destinationRect.width / 2.0F,
                               draw.destinationRect.height / 2.0F};
            }
            scene.sprites.push_back(std::move(draw));
        });

    registry.each<PlayerSlot, engine::Position, Extrapolation, Sprite>(
        [&](engine::Entity entity, PlayerSlot& slot, engine::Position& position,
            Extrapolation& motion, Sprite& sprite) {
            if (registry.hasComponent<Dying>(entity))
                return;
            const auto it = names.find(static_cast<std::uint32_t>(slot.index) + 1);
            gui::TextDraw text;
            text.text = it != names.end() ? it->second : "Player";
            text.color = {0, 0, 0, 255};
            text.position = {position.x + motion.offsetX - sprite.width / 4.F,
                             position.y + motion.offsetY - sprite.height / 2.F - 24.F};
            scene.sceneTexts.push_back(std::move(text));
        });
}

inline void addBossBar(gui::RenderScene& scene, const protocol::BossStateMessage& boss,
                       float animationSeconds)
{
    constexpr float Width = 900.F;
    constexpr float Height = 18.F;
    constexpr float Left = (1920.F - Width) / 2.F;
    constexpr float Top = 104.F;
    constexpr float Border = 3.F;
    const bool entering = boss.phase == 0;
    const bool enraged = boss.has(protocol::BossStateMessage::Enraged);

    const float fraction = static_cast<float>(boss.health) / static_cast<float>(boss.maxHealth);
    scene.sceneRectangles.push_back(
        {{{Left - Border, Top - Border}, Width + 2.F * Border, Height + 2.F * Border},
         {15, 15, 20, 230}});
    scene.sceneRectangles.push_back({{{Left, Top}, Width, Height}, {70, 60, 60, 255}});
    const float fill = entering ? Width * std::min(1.F, std::fmod(animationSeconds, 3.F) / 2.5F)
                                : Width * std::clamp(fraction, 0.F, 1.F);
    const gui::GuiColor color =
        enraged ? gui::GuiColor{235, 60, 45, 255} : gui::GuiColor{245, 175, 40, 255};
    scene.sceneRectangles.push_back({{{Left, Top}, fill, Height}, color});
    for (const std::uint8_t mark : boss.marks) {
        const float x = Left + Width * static_cast<float>(mark) / 100.F;
        scene.sceneRectangles.push_back(
            {{{x - 1.5F, Top - 6.F}, 3.F, Height + 12.F}, {240, 240, 240, 255}});
    }

    gui::TextDraw name;
    name.text = boss.name;
    name.fontSize = 26.F;
    name.color = {240, 240, 240, 255};
    name.position = {Left, Top - 34.F};
    scene.sceneTexts.push_back(std::move(name));
    if (!entering && boss.phaseCount > 1) {
        gui::TextDraw phase;
        phase.text = "Phase " + std::to_string(boss.phase) + "/" + std::to_string(boss.phaseCount);
        phase.fontSize = 22.F;
        phase.color = {200, 200, 200, 255};
        phase.position = {Left + Width - 110.F, Top - 30.F};
        scene.sceneTexts.push_back(std::move(phase));
    }
    if (entering && static_cast<int>(animationSeconds * 3.F) % 2 == 0) {
        gui::TextDraw warning;
        warning.text = "WARNING";
        warning.fontSize = 72.F;
        warning.spacing = 6.F;
        warning.color = {235, 60, 45, 255};
        warning.position = {1920.F / 2.F - 170.F, 1080.F / 2.F - 60.F};
        scene.sceneTexts.push_back(std::move(warning));
    }
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
