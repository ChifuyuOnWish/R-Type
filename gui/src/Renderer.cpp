#include "Renderer.hpp"

#include "ColorVision.hpp"
#include "detail/ResourceManagerData.hpp"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <stdexcept>

namespace rtype::gui {
namespace {

constexpr const char* colorFragment = R"glsl(#version 330
in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec3 colorRows[3];
out vec4 finalColor;

vec3 linearize(vec3 v)
{
    return mix(pow((v + 0.055) / 1.055, vec3(2.4)), v / 12.92,
               lessThanEqual(v, vec3(0.04045)));
}

vec3 encode(vec3 v)
{
    v = clamp(v, 0.0, 1.0);
    return mix(1.055 * pow(v, vec3(1.0 / 2.4)) - 0.055, v * 12.92,
               lessThanEqual(v, vec3(0.0031308)));
}

void main()
{
    vec4 sampleColor = texture(texture0, fragTexCoord);
    vec3 rgb = linearize(sampleColor.rgb);
    vec3 adjusted = vec3(dot(colorRows[0], rgb), dot(colorRows[1], rgb), dot(colorRows[2], rgb));
    finalColor = vec4(encode(adjusted), sampleColor.a);
}
)glsl";

constexpr const char* inkFragment = R"glsl(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform int inkMode;
uniform float flash;
uniform float dissolve;
uniform vec4 dissolveRect; // source rect in texture coordinates (u, v, w, h)
out vec4 finalColor;

void main()
{
    vec4 sampleColor = texture(texture0, fragTexCoord);
    vec4 result;
    if (inkMode == 0) {
        result = sampleColor * colDiffuse * fragColor;
    } else if (inkMode == 3) {
        float light = dot(sampleColor.rgb, vec3(0.299, 0.587, 0.114));
        vec3 rgb = mix(vec3(0.07), fragColor.rgb, smoothstep(0.15, 0.85, light));
        result = vec4(rgb, sampleColor.a * fragColor.a * colDiffuse.a);
    } else {
        if (inkMode == 1) {
            vec3 rgb = sampleColor.rgb;
            float chroma = max(max(rgb.r, rgb.g), rgb.b) - min(min(rgb.r, rgb.g), rgb.b);
            sampleColor.a *= smoothstep(0.105, 0.205, chroma);
        } else {
            vec2 stepSize = 2.0 / vec2(textureSize(texture0, 0));
            float a = sampleColor.a;
            a = max(a, texture(texture0, fragTexCoord + vec2(stepSize.x, 0.0)).a);
            a = max(a, texture(texture0, fragTexCoord - vec2(stepSize.x, 0.0)).a);
            a = max(a, texture(texture0, fragTexCoord + vec2(0.0, stepSize.y)).a);
            a = max(a, texture(texture0, fragTexCoord - vec2(0.0, stepSize.y)).a);
            sampleColor = vec4(vec3(0.07), mix(sampleColor.a, a, 0.72));
        }
        result = sampleColor * colDiffuse * fragColor;
    }
    if (dissolve > 0.0) {
        vec2 local = (fragTexCoord - dissolveRect.xy) / dissolveRect.zw;
        vec2 cell = floor(fragTexCoord * vec2(textureSize(texture0, 0)) / 5.0);
        float noise = fract(sin(dot(cell, vec2(12.9898, 78.233))) * 43758.5453);
        // The disintegration sweeps from left to right, roughened by noise.
        float front = 0.7 * clamp(local.x, 0.0, 1.0) + 0.3 * noise;
        float progress = dissolve * 1.1;
        if (front < progress)
            discard;
        // Thin burnt rim right ahead of the front.
        result.rgb = mix(result.rgb, vec3(0.07), 1.0 - smoothstep(0.0, 0.05, front - progress));
    }
    // Red pencil: stays visible on the cream paper, unlike white.
    result.rgb = mix(result.rgb, vec3(0.89, 0.27, 0.22), flash);
    finalColor = result;
}
)glsl";

bool prepareColorPass(detail::ColorPass& pass, int width, int height)
{
    if (pass.rows < 0 || width <= 0 || height <= 0)
        return false;
    if (pass.target.id != 0 &&
        (pass.target.texture.width != width || pass.target.texture.height != height)) {
        UnloadRenderTexture(pass.target);
        pass.target = {};
    }
    if (pass.target.id == 0)
        pass.target = LoadRenderTexture(width, height);
    return IsRenderTextureValid(pass.target);
}

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

    for (const auto* list : {&scene.rectangles, &scene.sceneRectangles}) {
        for (const auto& draw : *list) {
            const auto& bounds = draw.bounds;
            if (!finite({bounds.pos.x, bounds.pos.y, bounds.width, bounds.height}) ||
                bounds.width < 0 || bounds.height < 0)
                throw std::invalid_argument("Invalid UI rectangle");
        }
    }
    for (const auto& particle : scene.particles)
        if (!finite({particle.position.x, particle.position.y, particle.size}) ||
            particle.size < 0.0F)
            throw std::invalid_argument("Invalid particle");
    const int width = GetScreenWidth();
    const int height = GetScreenHeight();
    detail::ColorPass* pass = nullptr;
    if (scene.colorVision != ColorVisionMode::Off && scene.colorVisionStrength > 0.0F) {
        auto& cache = _resources._impl->colorPass;
        if (!cache) {
            cache = std::make_unique<detail::ColorPass>();
            cache->shader = LoadShaderFromMemory(nullptr, colorFragment);
            cache->rows = GetShaderLocation(cache->shader, "colorRows[0]");
        }
        if (prepareColorPass(*cache, width, height))
            pass = cache.get();
    }

    const detail::InkPass* ink = nullptr;
    if (std::any_of(sprites.begin(), sprites.end(), [](const auto& sprite) {
            return sprite.draw->ink != SpriteInk::Original || sprite.draw->flash > 0.0F ||
                   sprite.draw->dissolve > 0.0F;
        })) {
        auto& cache = _resources._impl->inkPass;
        if (!cache) {
            cache = std::make_unique<detail::InkPass>();
            cache->shader = LoadShaderFromMemory(nullptr, inkFragment);
            cache->modeLocation = GetShaderLocation(cache->shader, "inkMode");
        }
        if (cache->modeLocation >= 0)
            ink = cache.get();
    }

    BeginDrawing();
    if (pass != nullptr)
        BeginTextureMode(pass->target);
    ClearBackground(color(scene.clearColor));
    if (scene.background.enabled) {
        for (const auto& star : backgroundStars)
            DrawRectangleRec({star.x, star.y, 2.0F, 2.0F}, color(scene.background.starColor));
    }
    const int flashLocation = ink != nullptr ? GetShaderLocation(ink->shader, "flash") : -1;
    const int dissolveLocation = ink != nullptr ? GetShaderLocation(ink->shader, "dissolve") : -1;
    const int dissolveRectLocation =
        ink != nullptr ? GetShaderLocation(ink->shader, "dissolveRect") : -1;
    const auto drawSprite = [ink, flashLocation, dissolveLocation,
                             dissolveRectLocation](const PreparedSprite& sprite) {
        const auto& value = *sprite.draw;
        const bool inkOnly = ink != nullptr && (value.ink != SpriteInk::Original ||
                                                value.flash > 0.0F || value.dissolve > 0.0F);
        if (inkOnly) {
            const int mode = static_cast<int>(value.ink);
            const float flash = std::clamp(value.flash, 0.0F, 1.0F);
            BeginShaderMode(ink->shader);
            SetShaderValue(ink->shader, ink->modeLocation, &mode, SHADER_UNIFORM_INT);
            if (flashLocation >= 0)
                SetShaderValue(ink->shader, flashLocation, &flash, SHADER_UNIFORM_FLOAT);
            const float dissolve = std::clamp(value.dissolve, 0.0F, 1.0F);
            if (dissolveLocation >= 0)
                SetShaderValue(ink->shader, dissolveLocation, &dissolve, SHADER_UNIFORM_FLOAT);
            if (dissolveRectLocation >= 0) {
                const auto textureWidth = static_cast<float>(sprite.texture.width);
                const auto textureHeight = static_cast<float>(sprite.texture.height);
                const float rect[4] = {
                    value.sourceRect.pos.x / textureWidth, value.sourceRect.pos.y / textureHeight,
                    value.sourceRect.width / textureWidth, value.sourceRect.height / textureHeight};
                SetShaderValue(ink->shader, dissolveRectLocation, rect, SHADER_UNIFORM_VEC4);
            }
        }

        Rectangle source = rectangle(value.sourceRect);
        if (value.flipY)
            source.height = -source.height;
        DrawTexturePro(sprite.texture, source, rectangle(value.destinationRect),
                       {value.origin.x, value.origin.y}, value.rotation, color(value.tint));
        if (inkOnly)
            EndShaderMode();
    };
    for (const auto& sprite : sprites) {
        if (sprite.draw->layer < 10000)
            drawSprite(sprite);
    }
    for (const auto& particle : scene.particles) {
        const float half = particle.size / 2.0F;
        DrawRectangleRec(
            {particle.position.x - half, particle.position.y - half, particle.size, particle.size},
            color(particle.color));
    }
    if (pass != nullptr) {
        EndTextureMode();
        const auto matrix = detail::assistanceMatrix(scene.colorVision, scene.colorVisionStrength);
        const std::array coefficients{matrix[0][0], matrix[0][1], matrix[0][2],
                                      matrix[1][0], matrix[1][1], matrix[1][2],
                                      matrix[2][0], matrix[2][1], matrix[2][2]};
        const auto w = static_cast<float>(width);
        const auto h = static_cast<float>(height);
        BeginShaderMode(pass->shader);
        SetShaderValueV(pass->shader, pass->rows, coefficients.data(), SHADER_UNIFORM_VEC3, 3);
        DrawTexturePro(pass->target.texture, {0, 0, w, -h}, {0, 0, w, h}, {0, 0}, 0, WHITE);
        EndShaderMode();
    }
    for (const auto& sprite : sprites) {
        if (sprite.draw->layer >= 10000)
            drawSprite(sprite);
    }
    for (const auto& draw : scene.sceneRectangles)
        DrawRectangleRec(rectangle(draw.bounds), color(draw.color));
    drawTexts(sceneTexts);
    for (const auto& draw : scene.rectangles)
        DrawRectangleRec(rectangle(draw.bounds), color(draw.color));
    drawTexts(overlays);
    EndDrawing();
}

} // namespace rtype::gui
