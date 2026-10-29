#include "SettingsMenu.hpp"

#include "SettingsStore.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace rtype::gui {
namespace {

constexpr std::size_t rowCount = 20;
constexpr float firstPaperLine = 0.28F;
constexpr float paperLineSpacing = 0.10F;
constexpr std::array categoryNames{"Display", "Audio", "Controls", "Accessibility"};
constexpr std::array actionNames{"Up", "Down", "Left", "Right", "Fire"};
constexpr std::array keyNames{
    "None", "Up", "Down",  "Left",      "Right",      "Space",      "A",          "B", "C", "D",
    "E",    "F",  "G",     "H",         "I",          "J",          "K",          "L", "M", "N",
    "O",    "P",  "Q",     "R",         "S",          "T",          "U",          "V", "W", "X",
    "Y",    "Z",  "Enter", "Left Ctrl", "Right Ctrl", "Left Shift", "Right Shift"};
static_assert(keyNames.size() == guiKeyCount);
std::string percent(float value)
{
    return std::to_string(static_cast<int>(std::round(value * 100.0F))) + "%";
}

std::string rowText(std::size_t index, const GuiPreferences& preferences)
{
    switch (index) {
    case 0:
        return "Volume: " + percent(preferences.audio.volume);
    case 1:
        return std::string("Mute: ") + (preferences.audio.muted ? "On" : "Off");
    case 2:
        return std::string("Background motion: ") +
               (preferences.visuals.backgroundMotion ? "On" : "Off");
    case 3:
        return "UI text size: " + percent(preferences.visuals.textScale);
    case 4: {
        constexpr std::array names{"Off", "Red", "Green", "Blue", "Amber"};
        return "Scene tint: " +
               std::string(names[static_cast<std::size_t>(preferences.visuals.tint)]);
    }
    case 5:
        return "Tint opacity: " + percent(preferences.visuals.tintOpacity);
    case 15:
        return std::string("Show FPS: ") + (preferences.visuals.showFps ? "On" : "Off");
    case 11:
        return "Reset";
    case 12:
        return "Apply & save";
    case 13:
        return "Back";
    case 14:
        return "Quit game";
    default:
        return "Key " + std::string(actionNames[index - 6]) + ": " +
               keyNames[static_cast<std::size_t>(preferences.keyboard.keys[index - 6])];
    }
}

float measuredWidth(const std::string& value, float size, const SettingsMenu::TextMeasure& measure)
{
    return measure ? measure(value, size) : static_cast<float>(value.size()) * (size + 1.0F);
}

std::size_t previousCharacter(const std::string& value, std::size_t end)
{
    if (end == 0)
        return 0;
    --end;
    while (end > 0 && (static_cast<unsigned char>(value[end]) & 0xC0) == 0x80)
        --end;
    return end;
}

std::string clipped(const std::string& value, float width, float size,
                    const SettingsMenu::TextMeasure& measure)
{
    if (measuredWidth(value, size, measure) <= width)
        return value;
    if (measuredWidth("...", size, measure) > width)
        return {};
    auto end = value.size();
    while (end > 0 && measuredWidth(value.substr(0, end) + "...", size, measure) > width)
        end = previousCharacter(value, end);
    return value.substr(0, end) + "...";
}

std::vector<std::string> wrapped(const std::string& value, float width, float size,
                                 const SettingsMenu::TextMeasure& measure)
{
    std::vector<std::string> result;
    std::string remaining = value;
    while (!remaining.empty()) {
        auto end = remaining.size();
        const auto newline = remaining.find('\n');
        if (newline != std::string::npos)
            end = newline;
        if (end == 0) {
            result.emplace_back();
            remaining.erase(0, 1);
            continue;
        }
        while (end > 0 && measuredWidth(remaining.substr(0, end), size, measure) > width)
            end = previousCharacter(remaining, end);
        if (end == 0) {
            end = 1;
            while (end < remaining.size() &&
                   (static_cast<unsigned char>(remaining[end]) & 0xC0) == 0x80)
                ++end;
        }
        if (end < remaining.size() && end != newline) {
            const auto space = remaining.rfind(' ', end);
            if (space != std::string::npos && space > 0)
                end = space;
        }
        result.push_back(remaining.substr(0, end));
        remaining.erase(0, end);
        if (!remaining.empty() && remaining.front() == '\n')
            remaining.erase(0, 1);
        while (!remaining.empty() && remaining.front() == ' ')
            remaining.erase(0, 1);
    }
    return result;
}

void text(RenderScene& scene, std::string value, GuiPosition position, float size, GuiColor color)
{
    TextDraw draw;
    draw.text = std::move(value);
    draw.position = position;
    draw.fontSize = size;
    draw.color = color;
    scene.overlayTexts.push_back(std::move(draw));
}

template <class Value, std::size_t Size>
void replaceBinding(std::array<Value, Size>& bindings, std::size_t action, Value value)
{
    const auto old = bindings[action];
    if (value != Value::None) {
        for (std::size_t index = 0; index < Size; ++index) {
            if (index != action && bindings[index] == value)
                bindings[index] = old;
        }
    }
    bindings[action] = value;
}

} // namespace

void SettingsMenu::setTextMeasure(TextMeasure measure)
{
    _measure = std::move(measure);
}

void SettingsMenu::setArtwork(std::string name, GuiVector2 size)
{
    _artwork[std::move(name)] = size;
}
std::vector<std::size_t> SettingsMenu::categoryRows() const
{
    switch (_category) {
    case 0:
        return {3, 15};
    case 1:
        return {0, 1};
    case 2:
        return {6, 7, 8, 9, 10};
    default:
        return {2, 4, 5};
    }
}

void SettingsMenu::open(const GuiPreferences& preferences)
{
    SettingsStore::validate(preferences);
    _draft = preferences;
    _dirty = false;
    _category = 0;
    _selected = 3;
    _capture = 0;
    _notice.clear();
    _open = true;
}

void SettingsMenu::close() noexcept
{
    _open = false;
    _capture = 0;
}

void SettingsMenu::select(std::size_t row)
{
    if (row >= rowCount || isCapturing())
        return;
    _selected = row;
    if (row >= 16)
        _category = row - 16;
    else if (row == 3 || row == 15)
        _category = 0;
    else if (row <= 1)
        _category = 1;
    else if (row >= 6 && row <= 10)
        _category = 2;
    else if (row == 2 || row == 4 || row == 5)
        _category = 3;
}

void SettingsMenu::setNotice(std::string notice)
{
    _notice = std::move(notice);
}

MenuResult SettingsMenu::handle(const MenuInput& input)
{
    if (!_open)
        return MenuResult::None;
    if (isCapturing()) {
        if (input.cancelCapture) {
            _capture = 0;
            _notice = "Binding change cancelled.";
        } else if (input.key && static_cast<std::size_t>(*input.key) < guiKeyCount) {
            replaceBinding(_draft.keyboard.keys, _selected - 6, *input.key);
            _capture = 0;
            _dirty = true;
            _notice = "Binding changed. Apply and save to keep it.";
        }
        return MenuResult::None;
    }
    if (input.back) {
        close();
        return MenuResult::Closed;
    }
    if (input.category != 0) {
        _category = static_cast<std::size_t>(
            (static_cast<int>(_category) + (input.category < 0 ? -1 : 1) + 4) % 4);
        _selected = categoryRows().front();
        return MenuResult::None;
    }
    if (input.vertical != 0) {
        auto navigation = categoryRows();
        navigation.insert(navigation.end(), {11, 12, 13, 14});
        const auto found = std::find(navigation.begin(), navigation.end(), _selected);
        const int index =
            found == navigation.end() ? 0 : static_cast<int>(found - navigation.begin());
        const int count = static_cast<int>(navigation.size());
        _selected = navigation[static_cast<std::size_t>(
            (index + (input.vertical < 0 ? -1 : 1) + count) % count)];
        return MenuResult::None;
    }
    if (!input.activate && input.horizontal == 0)
        return MenuResult::None;
    const int direction = input.horizontal < 0 ? -1 : 1;
    _notice.clear();
    if (_selected >= 16) {
        _category = _selected - 16;
        _selected = categoryRows().front();
        return MenuResult::None;
    }
    switch (_selected) {
    case 0:
        _draft.audio.volume = std::clamp(
            std::round((_draft.audio.volume + static_cast<float>(direction) * 0.05F) * 100.0F) /
                100.0F,
            0.0F, 1.0F);
        break;
    case 1:
        _draft.audio.muted = !_draft.audio.muted;
        break;
    case 2:
        _draft.visuals.backgroundMotion = !_draft.visuals.backgroundMotion;
        break;
    case 3:
        _draft.visuals.textScale = std::clamp(
            _draft.visuals.textScale + static_cast<float>(direction) * 0.25F, 1.0F, 1.5F);
        break;
    case 4:
        _draft.visuals.tint =
            static_cast<TintMode>((static_cast<int>(_draft.visuals.tint) + direction + 5) % 5);
        break;
    case 5:
        _draft.visuals.tintOpacity = std::clamp(
            std::round((_draft.visuals.tintOpacity + static_cast<float>(direction) * 0.05F) *
                       100.0F) /
                100.0F,
            0.0F, 0.5F);
        break;
    case 15:
        _draft.visuals.showFps = !_draft.visuals.showFps;
        break;
    case 11:
        _draft = {};
        _notice = "Defaults restored. Apply to keep them.";
        break;
    case 12:
        _dirty = false;
        return MenuResult::Apply;
    case 13:
        close();
        return MenuResult::Closed;
    case 14:
        close();
        return MenuResult::Quit;
    default:
        if (input.horizontal < 0)
            _draft.keyboard.keys[_selected - 6] = GuiKey::None;
        else {
            _capture = 1;
            _notice = "Press a key. Escape or F1 cancels.";
        }
        break;
    }
    if (!isCapturing())
        _dirty = true;
    if (_notice.empty())
        _notice = "Unsaved changes. Apply and save to keep them.";
    return MenuResult::None;
}

std::vector<SettingsRow> SettingsMenu::rows(GuiVector2 viewport) const
{
    std::vector<SettingsRow> result;
    if (!std::isfinite(viewport.x) || !std::isfinite(viewport.y) || viewport.x < 640 ||
        viewport.y < 360)
        return result;
    const float sidebar = viewport.x * 0.23F;
    const float pitch = viewport.y * paperLineSpacing;
    const float firstLine = viewport.y * firstPaperLine;
    for (std::size_t category = 0; category < 4; ++category)
        result.push_back({16 + category,
                          {{16, firstLine - pitch + 4 + static_cast<float>(category) * pitch},
                           sidebar - 24,
                           pitch - 8},
                          categoryNames[category]});
    const auto options = categoryRows();
    for (std::size_t index = 0; index < options.size(); ++index) {
        const auto id = options[index];
        result.push_back(
            {id,
             {{sidebar + 24, firstLine - pitch + 4 + static_cast<float>(index) * pitch},
              viewport.x - sidebar - 56,
              pitch - 4},
             rowText(id, _draft),
             id <= 5 || id == 15});
    }
    const float gap = 12;
    const float buttonWidth = (viewport.x - 32 - 3 * gap) / 4;
    const float buttonHeight = std::clamp(viewport.y * 0.085F, 40.0F, 76.0F);
    for (std::size_t index = 0; index < 4; ++index)
        result.push_back({11 + index,
                          {{16 + static_cast<float>(index) * (buttonWidth + gap),
                            viewport.y - 16 - buttonHeight},
                           buttonWidth,
                           buttonHeight},
                          rowText(11 + index, _draft)});
    return result;
}

RenderScene SettingsMenu::makeScene(GuiVector2 viewport) const
{
    RenderScene scene;
    scene.clearColor = {248, 246, 235, 255};
    const float scale = _draft.visuals.textScale;
    const float size = std::clamp(viewport.y * 0.04F, 18.0F, 42.0F) * scale;
    const float sidebar = viewport.x * 0.23F;
    const float pitch = viewport.y * paperLineSpacing;
    const float firstLine = viewport.y * firstPaperLine;
    constexpr GuiColor ink{30, 30, 35, 255};
    const auto image = [&](const std::string& id, GuiRectangle bounds) {
        const auto found = _artwork.find(id);
        if (found == _artwork.end())
            return false;
        SpriteDraw sprite;
        sprite.textureId = "ui-" + id;
        sprite.sourceRect = {{0, 0}, found->second.x, found->second.y};
        sprite.destinationRect = bounds;
        sprite.layer = id == "settings-background" ? -1000 : 0;
        scene.sprites.push_back(std::move(sprite));
        return true;
    };
    if (!image("settings-background", {{0, 0}, viewport.x, viewport.y})) {
        for (float y = firstLine; y < viewport.y - 90; y += pitch)
            scene.rectangles.push_back(
                {{{sidebar + 12, y}, viewport.x - sidebar - 40, 1}, {155, 181, 206, 255}});
        scene.rectangles.push_back(
            {{{sidebar + 8, viewport.y * 0.18F}, 1, viewport.y * 0.60F}, {196, 122, 125, 255}});
    }
    const auto fitted = [&](const std::string& value, GuiRectangle bounds, float desired,
                            GuiColor color, bool centered) {
        float fontSize = desired;
        while (fontSize > 14 && measuredWidth(value, fontSize, _measure) > bounds.width - 16)
            fontSize -= 1;
        const auto displayed = clipped(value, bounds.width - 16, fontSize, _measure);
        const float width = measuredWidth(displayed, fontSize, _measure);
        const float x = bounds.pos.x + (centered ? (bounds.width - width) / 2 : 8);
        const float y = centered ? bounds.pos.y + (bounds.height - fontSize) / 2
                                 : bounds.pos.y + bounds.height - fontSize - 3;
        text(scene, displayed, {x, y}, fontSize, color);
    };
    text(scene, "Settings", {sidebar + 24, 18},
         std::clamp(viewport.y * 0.055F, 24.0F, 58.0F) * scale, ink);
    for (const auto& row : rows(viewport)) {
        const bool category = row.index >= 16;
        const bool action = row.index >= 11 && row.index <= 14;
        const bool selected = row.index == _selected;
        if (category || action) {
            constexpr std::array categoryImages{"settings-display", "settings-audio",
                                                "settings-controls", "settings-accessibility"};
            constexpr std::array actionImages{"reset", "save", "back", "quit"};
            const std::string art =
                category ? categoryImages[row.index - 16] : actionImages[row.index - 11];
            if (!image(art, row.bounds) && !image("button", row.bounds))
                scene.rectangles.push_back({row.bounds, category && row.index - 16 == _category
                                                            ? GuiColor{220, 216, 200, 255}
                                                            : GuiColor{255, 255, 255, 255}});
            if (category && row.index - 16 == _category)
                scene.rectangles.push_back(
                    {{{row.bounds.pos.x, row.bounds.pos.y}, 3, row.bounds.height}, ink});
            if (selected) {
                scene.rectangles.push_back(
                    {{{row.bounds.pos.x, row.bounds.pos.y}, row.bounds.width, 2}, ink});
                scene.rectangles.push_back(
                    {{{row.bounds.pos.x, row.bounds.pos.y + row.bounds.height - 2},
                      row.bounds.width,
                      2},
                     ink});
            }
            fitted(row.text, row.bounds, size * (category ? 0.8F : 0.85F), ink, true);
            continue;
        }
        if (selected)
            text(scene, ">",
                 {row.bounds.pos.x - 18, row.bounds.pos.y + row.bounds.height - size - 3}, size,
                 ink);
        const auto separator = row.text.find(": ");
        const bool hasValue = separator != std::string::npos;
        const float labelWidth = row.bounds.width * 0.60F;
        fitted(row.text.substr(0, separator), {row.bounds.pos, labelWidth, row.bounds.height}, size,
               ink, false);
        if (hasValue) {
            std::string value =
                isCapturing() && selected ? "Press a key..." : row.text.substr(separator + 2);
            const float padding = row.adjustable ? 36.0F : 0.0F;
            fitted(value,
                   {{row.bounds.pos.x + labelWidth + padding, row.bounds.pos.y},
                    row.bounds.width - labelWidth - 2 * padding,
                    row.bounds.height},
                   size, ink, false);
            if (row.adjustable) {
                fitted("<",
                       {{row.bounds.pos.x + labelWidth, row.bounds.pos.y}, 36, row.bounds.height},
                       size, ink, false);
                fitted(">",
                       {{row.bounds.pos.x + row.bounds.width - 36, row.bounds.pos.y},
                        36,
                        row.bounds.height},
                       size, ink, false);
            }
        }
    }
    const float buttonHeight = std::clamp(viewport.y * 0.085F, 40.0F, 76.0F);
    const float noticeSize = std::clamp(viewport.y * 0.025F, 12.0F, 20.0F);
    const auto message =
        !_notice.empty() ? _notice
        : _dirty         ? "Unsaved changes. Apply and save to keep them."
                         : "Tab: category   Up/Down: select   Left/Right: change   Enter: confirm";
    auto lines = wrapped(message, viewport.x - 48, noticeSize, _measure);
    const float noticeTop =
        viewport.y - 24 - buttonHeight - static_cast<float>(lines.size()) * (noticeSize + 2);
    for (std::size_t index = 0; index < lines.size(); ++index)
        text(scene, lines[index], {24, noticeTop + static_cast<float>(index) * (noticeSize + 2)},
             noticeSize, ink);
    return scene;
}

} // namespace rtype::gui
