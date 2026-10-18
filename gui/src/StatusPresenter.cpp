#include "StatusPresenter.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string_view>

namespace rtype::gui {
namespace {

constexpr GuiColor textColor{240, 244, 248, 255};
constexpr GuiColor messageColor{216, 224, 232, 255};

std::size_t characterLength(std::string_view text, std::size_t offset)
{
    const auto first = static_cast<unsigned char>(text[offset]);
    const std::size_t length = first < 0x80             ? 1
                               : (first & 0xE0) == 0xC0 ? 2
                               : (first & 0xF0) == 0xE0 ? 3
                               : (first & 0xF8) == 0xF0 ? 4
                                                        : 1;
    if (offset + length > text.size())
        return 1;
    for (std::size_t index = 1; index < length; ++index) {
        if ((static_cast<unsigned char>(text[offset + index]) & 0xC0) != 0x80)
            return 1;
    }
    return length;
}

std::size_t characters(std::string_view text)
{
    std::size_t count = 0;
    for (std::size_t offset = 0; offset < text.size(); ++count)
        offset += characterLength(text, offset);
    return count;
}

std::string prefix(std::string_view text, std::size_t count)
{
    std::size_t offset = 0;
    while (offset < text.size() && count > 0) {
        offset += characterLength(text, offset);
        --count;
    }
    return std::string(text.substr(0, offset));
}

std::vector<std::string> wrap(std::string_view text, std::size_t columns)
{
    std::vector<std::string> lines;
    std::string current;
    std::size_t width = 0;
    for (std::size_t offset = 0; offset < text.size();) {
        const auto length = characterLength(text, offset);
        const auto character = text.substr(offset, length);
        offset += length;
        if (character == "\n") {
            lines.push_back(current);
            current.clear();
            width = 0;
            continue;
        }
        if (length == 1 && static_cast<unsigned char>(character.front()) < 32 && character != "\t")
            continue;
        if (width >= columns) {
            const auto space = current.find_last_of(' ');
            if (space != std::string::npos && space > 0) {
                lines.push_back(current.substr(0, space));
                current.erase(0, space + 1);
                width = characters(current);
            } else {
                lines.push_back(current);
                current.clear();
                width = 0;
            }
        }
        if (current.empty() && (character == " " || character == "\t"))
            continue;
        current += character == "\t" ? " " : character;
        ++width;
    }
    if (!current.empty())
        lines.push_back(current);
    return lines;
}

struct StatusDescription
{
    const char* title;
    const char* message;
};

StatusDescription describe(GuiStatus status)
{
    switch (status) {
    case GuiStatus::Connecting:
        return {"Connecting", "Waiting for the server to accept the connection."};
    case GuiStatus::Rejected:
        return {"Connection rejected", "Check the connection settings and try again."};
    case GuiStatus::Disconnected:
        return {"Disconnected", "The connection was lost. The game view is hidden."};
    default:
        return {"Error", "An operation failed. Check the client logs for details."};
    }
}

TextDraw line(const std::string& text, GuiPosition position, float fontSize, GuiColor color)
{
    TextDraw result;
    result.text = text;
    result.position = position;
    result.fontSize = fontSize;
    result.color = color;
    return result;
}

std::size_t columnsFor(float width, float fontSize)
{
    return static_cast<std::size_t>(std::max(1.0F, std::floor(width / (fontSize + 1.0F))));
}

} // namespace

RenderScene StatusPresenter::makeScene(const StatusFeedback& feedback, GuiVector2 viewport)
{
    if (!std::isfinite(viewport.x) || !std::isfinite(viewport.y) || viewport.x < 0.0F ||
        viewport.y < 0.0F)
        throw std::invalid_argument("Status viewport must be finite and non-negative");
    RenderScene scene;
    scene.clearColor = {16, 20, 28, 255};
    if (feedback.status == GuiStatus::None || viewport.x == 0.0F || viewport.y == 0.0F)
        return scene;

    const auto description = describe(feedback.status);
    const float margin = std::min(24.0F, std::min(viewport.x, viewport.y) / 8.0F);
    const float width = std::min(900.0F, viewport.x - 2.0F * margin);
    const float x = (viewport.x - width) / 2.0F;
    const float titleSize = std::clamp(viewport.x / 20.0F, 16.0F, 32.0F);
    const float bodySize = std::clamp(viewport.x / 28.0F, 14.0F, 24.0F);
    const float hintSize = std::clamp(viewport.x / 36.0F, 12.0F, 18.0F);
    float y = margin;
    const auto titles = wrap(description.title, columnsFor(width, titleSize));
    for (const auto& title : titles) {
        if (y + titleSize > viewport.y - margin)
            break;
        scene.overlayTexts.push_back(line(title, {x, y}, titleSize, textColor));
        y += titleSize + 8.0F;
    }
    y += 16.0F;

    const auto hints = wrap("Escape: close window", columnsFor(width, hintSize));
    const float hintY = viewport.y - margin - static_cast<float>(hints.size()) * (hintSize + 8.0F);
    const bool showHint = hintY >= y;
    const float bodyEnd = showHint ? hintY - 16.0F : viewport.y - margin;
    const auto columns = columnsFor(width, bodySize);
    const std::string_view message = feedback.message.empty()
                                         ? std::string_view(description.message)
                                         : std::string_view(feedback.message);
    std::string bounded = prefix(message, 2048);
    if (bounded.size() < message.size())
        bounded += "...";
    auto messages = wrap(bounded, columns);
    const auto availableLines =
        static_cast<std::size_t>(std::max(0.0F, std::floor((bodyEnd - y) / (bodySize + 8.0F))));
    if (messages.size() > availableLines) {
        messages.resize(availableLines);
        if (!messages.empty())
            messages.back() = prefix(messages.back(), columns > 3 ? columns - 3 : 0) +
                              std::string(std::min<std::size_t>(columns, 3), '.');
    }
    for (const auto& messageLine : messages) {
        scene.overlayTexts.push_back(line(messageLine, {x, y}, bodySize, messageColor));
        y += bodySize + 8.0F;
    }
    if (showHint) {
        y = hintY;
        for (const auto& hint : hints) {
            scene.overlayTexts.push_back(line(hint, {x, y}, hintSize, textColor));
            y += hintSize + 8.0F;
        }
    }
    return scene;
}

} // namespace rtype::gui
