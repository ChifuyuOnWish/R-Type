#include "Renderer.hpp"

#include "detail/ResourceManagerData.hpp"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <stdexcept>

namespace rtype::gui {
namespace {

bool finite(std::initializer_list<float> values)
{
    return std::all_of(values.begin(), values.end(),
                       [](float value) { return std::isfinite(value); });
}

Rectangle rectangle(const GuiRectangle& value)
{
    return {value.pos.x, value.pos.y, value.width, value.height};
}

Color color(const GuiColor& value)
{
    return {value.red, value.green, value.blue, value.alpha};
}

void validateSprite(const SpriteDraw& sprite, const Texture2D& texture)
{
    const auto& source = sprite.sourceRect;
    const auto& destination = sprite.destinationRect;
    if (!finite({source.pos.x, source.pos.y, source.width, source.height, destination.pos.x,
                 destination.pos.y, destination.width, destination.height, sprite.origin.x,
                 sprite.origin.y, sprite.rotation}))
        throw std::invalid_argument("Sprite coordinates must be finite: " + sprite.textureId);
    if (source.width <= 0.0F || source.height <= 0.0F || destination.width <= 0.0F ||
        destination.height <= 0.0F)
        throw std::invalid_argument("Sprite dimensions must be positive: " + sprite.textureId);
    if (source.pos.x < 0.0F || source.pos.y < 0.0F ||
        source.width > static_cast<float>(texture.width) ||
        source.height > static_cast<float>(texture.height) ||
        source.pos.x > static_cast<float>(texture.width) - source.width ||
        source.pos.y > static_cast<float>(texture.height) - source.height)
        throw std::invalid_argument("Sprite source rectangle exceeds texture: " + sprite.textureId);
}

struct PreparedSprite
{
    Texture2D texture;
    const SpriteDraw* draw;
};

struct PreparedText
{
    Font font;
    const TextDraw* draw;
};

void drawTexts(const std::vector<PreparedText>& texts)
{
    for (const auto& text : texts) {
        const auto& value = *text.draw;
        DrawTextEx(text.font, value.text.c_str(), {value.position.x, value.position.y},
                   value.fontSize, value.spacing, color(value.color));
    }
}

} // namespace

Renderer::Renderer(const ResourceManager& resources) : _resources(resources) {}

void Renderer::render(const RenderScene& scene, std::span<const GuiPosition> backgroundStars) const
{
    if (!IsWindowReady())
        throw std::logic_error("Rendering requires an open window");

    // Resolve and validate everything before starting a drawing frame.
    std::vector<PreparedSprite> sprites;
    sprites.reserve(scene.sprites.size());
    for (const auto& sprite : scene.sprites) {
        const auto found = _resources._impl->textures.find(sprite.textureId);
        if (found == _resources._impl->textures.end())
            throw std::out_of_range("Unknown texture ID: " + sprite.textureId);
        const auto texture = found->second->value;
        validateSprite(sprite, texture);
        sprites.push_back({texture, &sprite});
    }
    std::stable_sort(sprites.begin(), sprites.end(), [](const auto& left, const auto& right) {
        return left.draw->layer < right.draw->layer;
    });

    const auto prepareTexts = [this](const std::vector<TextDraw>& source) {
        std::vector<PreparedText> result;
        result.reserve(source.size());
        for (const auto& text : source) {
            if (!finite({text.position.x, text.position.y, text.fontSize, text.spacing}) ||
                text.fontSize <= 0.0F)
                throw std::invalid_argument("Text geometry must be finite with positive font size");
            Font font{};
            if (text.fontId.empty()) {
                font = GetFontDefault();
            } else {
                const auto found = _resources._impl->fonts.find(text.fontId);
                if (found == _resources._impl->fonts.end())
                    throw std::out_of_range("Unknown font ID: " + text.fontId);
                font = found->second->value;
            }
            if (!IsFontValid(font) || !IsTextureValid(font.texture))
                throw std::runtime_error("Font is not ready: " + text.fontId);
            result.push_back({font, &text});
        }
        return result;
    };
    const auto sceneTexts = prepareTexts(scene.sceneTexts);
    const auto overlays = prepareTexts(scene.overlayTexts);

    BeginDrawing();
    ClearBackground(color(scene.clearColor));
    if (scene.background.enabled) {
        for (const auto& star : backgroundStars)
            DrawRectangleRec({star.x, star.y, 2.0F, 2.0F}, color(scene.background.starColor));
    }
    for (const auto& sprite : sprites) {
        const auto& value = *sprite.draw;
        DrawTexturePro(sprite.texture, rectangle(value.sourceRect),
                       rectangle(value.destinationRect), {value.origin.x, value.origin.y},
                       value.rotation, color(value.tint));
    }
    drawTexts(sceneTexts);
    drawTexts(overlays);
    EndDrawing();
}

} // namespace rtype::gui
