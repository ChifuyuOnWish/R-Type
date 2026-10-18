#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace rtype::gui {

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
    bool audioEnabled = true;
    std::string settingsPath;
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

enum class GuiKey : std::size_t
{
    None,
    Up,
    Down,
    Left,
    Right,
    Space,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    Enter,
    LeftControl,
    RightControl,
    LeftShift,
    RightShift,
    Count
};

inline constexpr std::size_t guiKeyCount = static_cast<std::size_t>(GuiKey::Count);

struct KeyBindings
{
    std::array<GuiKey, playerActionCount> keys{GuiKey::Up, GuiKey::Down, GuiKey::Left,
                                               GuiKey::Right, GuiKey::Space};
};

struct AudioSettings
{
    float volume = 1.0F;
    bool muted = false;
};

enum class TintMode
{
    Off,
    Red,
    Green,
    Blue,
    Amber
};

struct VisualSettings
{
    bool backgroundMotion = true;
    float textScale = 1.0F;
    TintMode tint = TintMode::Off;
    float tintOpacity = 0.15F;
};

struct GuiPreferences
{
    KeyBindings keyboard;
    AudioSettings audio{0.5F, false};
    VisualSettings visuals;
};

struct RectangleDraw
{
    GuiRectangle bounds;
    GuiColor color;
};

struct SoundEvent
{
    std::string soundId;
};

struct BackgroundConfig
{
    bool enabled = false;
    float scrollSpeed = 40.0F;
    GuiColor starColor{180, 190, 210, 255};
};

enum class GuiStatus
{
    None,
    Connecting,
    Rejected,
    Disconnected,
    Error
};

struct StatusFeedback
{
    GuiStatus status = GuiStatus::None;
    std::string message;
};

struct RenderScene
{
    GuiColor clearColor{0, 0, 0, 255};
    std::vector<SpriteDraw> sprites;
    std::vector<TextDraw> sceneTexts;
    std::vector<TextDraw> overlayTexts;
    BackgroundConfig background;
    StatusFeedback feedback;
    GuiColor sceneTint{0, 0, 0, 0};
    std::vector<RectangleDraw> rectangles;
    GuiVector2 worldSize;
};

} // namespace rtype::gui
