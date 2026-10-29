#pragma once

#include "GuiTypes.hpp"
#include "InputReader.hpp"

#include <functional>
#include <map>

namespace rtype::gui {

struct FrontendControl
{
    int id = 0;
    GuiRectangle bounds;
    std::string label;
    std::string artwork;
    bool enabled = true;
};

class FrontendMenu
{
public:
    using TextMeasure = std::function<float(const std::string&, float)>;
    void reset();
    void setState(const FrontendState& state);
    void setTextMeasure(TextMeasure measure);
    void setFont(std::string id);
    void setArtwork(std::string name, GuiVector2 size);
    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] std::vector<FrontendControl> controls(GuiVector2 viewport, float textScale) const;
    [[nodiscard]] FrontendAction handle(const InputFrame& frame, GuiVector2 viewport, float scale);
    [[nodiscard]] RenderScene makeScene(GuiVector2 viewport, float scale) const;
    [[nodiscard]] bool takeSettingsRequest() noexcept;

private:
    [[nodiscard]] FrontendAction activate(int id);
    void text(RenderScene& scene, const std::string& value, GuiRectangle bounds, float size,
              GuiColor color) const;
    FrontendState _state;
    ConnectionDetails _entry;
    std::string _port = "4242";
    std::string _notice;
    std::string _font;
    std::map<std::string, GuiVector2> _artwork;
    TextMeasure _measure;
    bool _initialized = false;
    bool _settingsRequested = false;
    int _selected = 0;
    int _hovered = -1;
};

} // namespace rtype::gui
