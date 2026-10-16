#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace rtype::gui {

// Preserve both fields when translating an engine entity identifier.
struct GuiEntityId
{
    std::uint32_t index = 0;
    std::uint32_t generation = 0;

    bool operator==(const GuiEntityId&) const noexcept = default;
};

struct GuiVector2
{
    float x = 0.0F;
    float y = 0.0F;
};

using GuiPosition = GuiVector2;

struct GuiRectangle
{
    GuiPosition pos;
    float width = 0.0F;
    float height = 0.0F;
};

struct GuiColor
{
    std::uint8_t red = 255;
    std::uint8_t green = 255;
    std::uint8_t blue = 255;
    std::uint8_t alpha = 255;
};

struct GuiConfig
{
    int width = 1280;
    int height = 720;
    std::string title = "R-Type";
    int targetFps = 60;
    bool resizable = true;
};

struct SpriteDraw
{
    GuiEntityId entityId;
    std::string textureId;
    GuiRectangle sourceRect;
    GuiRectangle destinationRect;
    GuiVector2 origin;
    float rotation = 0.0F;
    GuiColor tint;
    int layer = 0;
};

struct TextDraw
{
    std::string fontId;
    std::string text;
    GuiPosition position;
    float fontSize = 20.0F;
    float spacing = 1.0F;
    GuiColor color;
};

enum class PlayerAction : std::size_t
{
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Fire,
    Count
};

inline constexpr std::size_t playerActionCount = static_cast<std::size_t>(PlayerAction::Count);

struct PlayerActions
{
    std::array<bool, playerActionCount> heldActions{};
    std::array<bool, playerActionCount> pressedActions{};
    std::array<bool, playerActionCount> releasedActions{};
};

struct SoundEvent
{
    std::string soundId;
};

struct RenderScene
{
    GuiColor clearColor{0, 0, 0, 255};
    std::vector<SpriteDraw> sprites;
    std::vector<TextDraw> sceneTexts;
    std::vector<TextDraw> overlayTexts;
};

} // namespace rtype::gui
