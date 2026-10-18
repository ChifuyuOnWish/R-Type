#include "SettingsMenu.hpp"

#include "SettingsStore.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace rtype::gui {
namespace {

constexpr std::size_t rowCount = 15;
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
    case 11:
        return "Restore defaults";
    case 12:
        return "Apply and save";
    case 13:
        return "Back";
    case 14:
        return "Quit application";
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
            // A single glyph wider than the viewport must still make progress.
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

std::vector<std::string> SettingsMenu::footerLines(GuiVector2 viewport) const
{
    const float x = std::max(20.0F, (viewport.x - 900.0F) / 2.0F);
    const float width = std::max(1.0F, viewport.x - x * 2.0F);
    const float size = 14.0F * _draft.visuals.textScale;
    std::vector<std::string> result;
    for (const auto& value : std::array<std::string, 3>{
             "Up/Down: select | L/R: edit",
             isCapturing() ? "Esc/F1: cancel" : "Enter: edit | Esc: back",
             _notice.empty() ? "Apply and save keeps changes; Back cancels the draft." : _notice}) {
        auto lines = wrapped(value, width, size, _measure);
        result.insert(result.end(), lines.begin(), lines.end());
    }
    return result;
}

void SettingsMenu::open(const GuiPreferences& preferences)
{
    SettingsStore::validate(preferences);
    _draft = preferences;
    _selected = 0;
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
    if (row < rowCount && !isCapturing())
        _selected = row;
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
            _notice = "Binding changed. Apply and save to keep it.";
        }
        return MenuResult::None;
    }
    if (input.back) {
        close();
        return MenuResult::Closed;
    }
    if (input.vertical != 0) {
        const int next = static_cast<int>(_selected) + (input.vertical < 0 ? -1 : 1);
        _selected = static_cast<std::size_t>((next + static_cast<int>(rowCount)) %
                                             static_cast<int>(rowCount));
        return MenuResult::None;
    }
    if (!input.activate && input.horizontal == 0)
        return MenuResult::None;
    const int direction = input.horizontal < 0 ? -1 : 1;
    _notice.clear();
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
    case 11:
        _draft = {};
        _notice = "Defaults restored. Apply to keep them.";
        break;
    case 12:
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
    if (_notice.empty())
        _notice = "Unsaved changes. Apply and save to keep them.";
    return MenuResult::None;
}

std::vector<SettingsRow> SettingsMenu::rows(GuiVector2 viewport) const
{
    std::vector<SettingsRow> result;
    if (!std::isfinite(viewport.x) || !std::isfinite(viewport.y) || viewport.x < 100.0F ||
        viewport.y < 100.0F)
        return result;
    const float scale = _draft.visuals.textScale;
    const float top = 64.0F * scale;
    const float height = 50.0F * scale;
    const float footerHeight =
        (static_cast<float>(footerLines(viewport).size()) * 22.0F + 22.0F) * scale;
    const float available = std::max(1.0F, std::floor((viewport.y - top - footerHeight) / height));
    const auto visible = std::min(rowCount, static_cast<std::size_t>(available));
    const std::size_t first = _selected < visible ? 0 : _selected - visible + 1;
    const float width = std::min(900.0F, viewport.x - 40.0F);
    const float x = (viewport.x - width) / 2.0F;
    for (std::size_t index = first; index < std::min(rowCount, first + visible); ++index)
        result.push_back(
            {index,
             {{x, top + static_cast<float>(index - first) * height}, width, height - 2.0F},
             rowText(index, _draft)});
    return result;
}

RenderScene SettingsMenu::makeScene(GuiVector2 viewport) const
{
    RenderScene scene;
    scene.clearColor = {12, 16, 22, 255};
    const float scale = _draft.visuals.textScale;
    const float fontSize = 18.0F * scale;
    const float x = std::max(20.0F, (viewport.x - 900.0F) / 2.0F);
    constexpr GuiColor white{245, 245, 245, 255};
    text(scene, "Settings", {x, 16.0F}, 28.0F * scale, white);
    for (const auto& row : rows(viewport)) {
        const bool selected = row.index == _selected;
        const GuiColor background =
            selected ? GuiColor{36, 66, 96, 255} : GuiColor{22, 28, 36, 255};
        scene.rectangles.push_back({row.bounds, background});
        const auto separator = row.text.find(": ");
        const bool hasValue = separator != std::string::npos;
        const auto label = (selected ? "> " : "  ") + row.text.substr(0, separator);
        const float labelWidth = row.bounds.width - ((row.index <= 5) ? 86.0F : 8.0F);
        const auto ink = white;
        text(scene, clipped(label, labelWidth, 18.0F * scale, _measure),
             {row.bounds.pos.x + 4.0F, row.bounds.pos.y + (hasValue ? 2.0F : 12.0F) * scale},
             18.0F * scale, ink);
        if (hasValue) {
            const float valueWidth = row.bounds.width - ((row.index <= 5) ? 86.0F : 24.0F);
            text(scene,
                 clipped(row.text.substr(separator + 2), valueWidth, 14.0F * scale, _measure),
                 {row.bounds.pos.x + 20.0F, row.bounds.pos.y + 25.0F * scale}, 14.0F * scale, ink);
        }
        if ((row.index <= 5)) {
            text(scene, "<", {row.bounds.pos.x + row.bounds.width - 70.0F, row.bounds.pos.y + 3.0F},
                 fontSize, white);
            text(scene, ">", {row.bounds.pos.x + row.bounds.width - 32.0F, row.bounds.pos.y + 3.0F},
                 fontSize, white);
        }
    }
    const float small = 14.0F * scale;
    const auto footer = footerLines(viewport);
    float y = viewport.y - 12.0F * scale - static_cast<float>(footer.size()) * 22.0F * scale;
    for (const auto& value : footer) {
        text(scene, value, {x, y}, small, white);
        y += 22.0F * scale;
    }
    return scene;
}

} // namespace rtype::gui
