#pragma once

#include "GuiTypes.hpp"

#include <functional>
#include <optional>

namespace rtype::gui {

struct MenuInput
{
    int vertical = 0;
    int horizontal = 0;
    bool activate = false;
    bool back = false;
    bool cancelCapture = false;
    std::optional<GuiKey> key;
};

enum class MenuResult
{
    None,
    Apply,
    Closed,
    Quit
};

struct SettingsRow
{
    std::size_t index = 0;
    GuiRectangle bounds;
    std::string text;
};

class SettingsMenu
{
public:
    using TextMeasure = std::function<float(const std::string&, float)>;
    void setTextMeasure(TextMeasure measure);
    void open(const GuiPreferences& preferences);
    void close() noexcept;
    [[nodiscard]] bool isOpen() const noexcept { return _open; }
    [[nodiscard]] bool isCapturing() const noexcept { return _capture != 0; }
    [[nodiscard]] const GuiPreferences& draft() const noexcept { return _draft; }
    [[nodiscard]] std::size_t selected() const noexcept { return _selected; }
    void select(std::size_t row);
    void setNotice(std::string notice);
    [[nodiscard]] MenuResult handle(const MenuInput& input);
    [[nodiscard]] std::vector<SettingsRow> rows(GuiVector2 viewport) const;
    [[nodiscard]] RenderScene makeScene(GuiVector2 viewport) const;

private:
    [[nodiscard]] std::vector<std::string> footerLines(GuiVector2 viewport) const;
    TextMeasure _measure;
    bool _open = false;
    int _capture = 0;
    std::size_t _selected = 0;
    GuiPreferences _draft;
    std::string _notice;
};

} // namespace rtype::gui
