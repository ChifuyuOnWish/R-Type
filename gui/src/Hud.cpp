#include "Hud.hpp"

#include <algorithm>

namespace rtype::gui {
namespace {

constexpr GuiColor ink{30, 30, 35, 255};

std::string spaced(std::uint32_t v)
{
    std::string d = std::to_string(v), out;
    for (std::size_t i = 0; i < d.size(); ++i) {
        if (i > 0 && (d.size() - i) % 3 == 0)
            out += ' ';
        out += d[i];
    }
    return out;
}

void text(RenderScene& s, const std::string& t, GuiPosition p, float size, GuiColor c = ink)
{
    TextDraw d;
    d.text = t;
    d.position = p;
    d.fontSize = size;
    d.color = c;
    s.overlayTexts.push_back(d);
}

void sprite(RenderScene& s, const std::string& id, GuiVector2 src, GuiRectangle dst, int layer,
            GuiColor tint)
{
    SpriteDraw d;
    d.textureId = id;
    d.sourceRect = {{0, 0}, src.x, src.y};
    d.destinationRect = dst;
    d.tint = tint;
    d.layer = layer;
    s.sprites.push_back(d);
}

float fit(const HudPresenter::TextMeasure& m, const std::string& t, float size, float maxW)
{
    while (size > 10.0F && m(t, size) > maxW)
        size -= 1.0F;
    return size;
}

} // namespace

void HudPresenter::append(RenderScene& scene, const GuiRectangle& button, float scale,
                          const TextMeasure& m)
{
    if (!scene.hud.visible)
        return;
    const std::string value = spaced(scene.hud.teamScore);
    const std::string combo = "COMBO " + std::to_string(scene.hud.combo) + "x";
    const float valueSize = fit(m, value, 40.0F * scale, button.pos.x - 400.0F * scale);
    const float centerX = button.pos.x - 16.0F - std::max(m(value, valueSize), 90.0F) / 2.0F;
    float y = button.pos.y + 10.0F * scale;

    const auto centered = [&](const std::string& t, float size) {
        text(scene, t, {centerX - m(t, size) / 2.0F, y}, size);
    };
    centered("SCORE", 14.0F * scale);
    y += 24.0F * scale;
    centered(value, valueSize);
    y += valueSize + 4.0F;
    if (scene.hud.combo > 1)
        centered(combo, 13.0F * scale);
}

void HudPresenter::appendPlayers(RenderScene& scene, GuiVector2 viewport, float scale,
                                 const TextMeasure& m, const TextureSize& texSize,
                                 GuiPosition origin)
{
    const auto& players = scene.hud.players;
    if (!scene.hud.visible || players.empty())
        return;

    const float gap = 10.0F;
    const float w = std::min(
        260.0F * scale,
        (viewport.x - origin.x - 280.0F * scale) / static_cast<float>(players.size()) - gap);
    const float h = 76.0F * scale;
    if (w < 80.0F)
        return;

    const auto paper = texSize("ui-paper-button");
    const auto full = texSize("ui-heart-full");
    const auto empty = texSize("ui-heart-empty");
    const float heart = 22.0F * scale;

    float x = origin.x;
    for (const auto& p : players) {
        const bool ko = p.health == 0;
        const auto alpha = static_cast<std::uint8_t>(ko ? 130 : 255);
        const GuiColor tint{255, 255, 255, alpha};
        const GuiColor col{30, 30, 35, alpha};
        const float top = origin.y;

        if (paper)
            sprite(scene, "ui-paper-button", *paper, {{x, top}, w, h}, 10000, tint);

        const std::string tag = ko ? "KO" : (p.isYou ? "YOU" : "");
        const float tagSize = 12.0F * scale;
        const float nameSize = fit(m, p.name, 18.0F * scale, w - 36.0F - m(tag, tagSize));
        text(scene, p.name, {x + 14.0F, top + 8.0F * scale}, nameSize, col);
        text(scene, tag, {x + w - 14.0F - m(tag, tagSize), top + 10.0F * scale}, tagSize, col);

        const float rowY = top + h - heart - 10.0F * scale;
        for (unsigned i = 0; i < p.maxHealth; ++i) {
            const auto& tex = i < p.health ? full : empty;
            if (tex)
                sprite(scene, i < p.health ? "ui-heart-full" : "ui-heart-empty", *tex,
                       {{x + 14.0F + static_cast<float>(i) * (heart + 4.0F), rowY}, heart, heart},
                       10001, tint);
        }
        const std::string score = spaced(p.score);
        const float room = w - 28.0F - static_cast<float>(p.maxHealth) * (heart + 4.0F);
        const float size = fit(m, score, 22.0F * scale, room);
        text(scene, score, {x + w - 14.0F - m(score, size), rowY + (heart - size) / 2.0F}, size,
             col);

        x += w + gap;
    }
}
} // namespace rtype::gui