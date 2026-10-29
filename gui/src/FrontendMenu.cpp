#include "FrontendMenu.hpp"

#include <algorithm>
#include <charconv>
#include <utility>

namespace rtype::gui {
namespace {
bool contains(const GuiRectangle& bounds, GuiPosition point)
{
    return point.x >= bounds.pos.x && point.y >= bounds.pos.y &&
           point.x < bounds.pos.x + bounds.width && point.y < bounds.pos.y + bounds.height;
}
float layoutScale(GuiVector2 viewport)
{
    return std::min(viewport.x / 1280.0F, viewport.y / 720.0F);
}
bool validAddress(const std::string& host)
{
    std::size_t start = 0;
    for (int part = 0; part < 4; ++part) {
        const auto end = host.find('.', start);
        if ((part < 3 && end == std::string::npos) || (part == 3 && end != std::string::npos))
            return false;
        const auto count = (end == std::string::npos ? host.size() : end) - start;
        if (count == 0 || count > 3)
            return false;
        unsigned octet = 0;
        const auto parsed =
            std::from_chars(host.data() + start, host.data() + start + count, octet);
        if (parsed.ec != std::errc{} || parsed.ptr != host.data() + start + count || octet > 255)
            return false;
        start += count + 1;
    }
    return true;
}
FrontendAction command(FrontendCommand value)
{
    FrontendAction result;
    result.command = value;
    return result;
}
} // namespace

void FrontendMenu::reset()
{
    *this = FrontendMenu{};
}
void FrontendMenu::setState(const FrontendState& state)
{
    if (!_initialized) {
        _entry = state.connection;
        _port = std::to_string(_entry.port);
        _initialized = true;
    }
    if (_state.page != state.page) {
        _selected = state.page == FrontendPage::Connection ? 1
                    : state.page == FrontendPage::Lobby    ? 10
                                                           : 0;
        _notice.clear();
    }
    _state = state;
}
void FrontendMenu::setTextMeasure(TextMeasure measure)
{
    _measure = std::move(measure);
}
void FrontendMenu::setFont(std::string id)
{
    _font = std::move(id);
}
void FrontendMenu::setArtwork(std::string name, GuiVector2 size)
{
    _artwork[std::move(name)] = size;
}
bool FrontendMenu::active() const noexcept
{
    return _state.page != FrontendPage::Gameplay;
}
bool FrontendMenu::takeSettingsRequest() noexcept
{
    return std::exchange(_settingsRequested, false);
}

std::vector<FrontendControl> FrontendMenu::controls(GuiVector2 viewport, float) const
{
    const float s = layoutScale(viewport);
    const float width = std::min(viewport.x - 32.0F, std::max(260.0F, 560.0F * s));
    const float left = (viewport.x - width) / 2.0F;
    const float height = std::max(36.0F, std::min(68.0F, viewport.y * 0.105F));
    const float gap = std::max(8.0F, 16.0F * s);
    const float top = viewport.y * 0.30F;
    const float side = std::max(36.0F, 64.0F * s);
    std::vector<FrontendControl> result{
        {0, {{viewport.x - side - 16.0F, 12.0F}, side, side}, "Settings", "settings"},
        {9, {{16.0F, viewport.y - 44.0F * s - 12.0F}, 100.0F * s, 44.0F * s}, "Quit", "quit"}};
    const auto row = [&](int id, int index, std::string label, std::string art,
                         bool enabled = true) {
        result.push_back({id,
                          {{left, top + static_cast<float>(index) * (height + gap)}, width, height},
                          std::move(label),
                          std::move(art),
                          enabled});
    };
    if (_state.page == FrontendPage::Connection) {
        row(1, 0, "Name: " + _entry.name, "name-field");
        row(2, 1, "IP: " + _entry.host, "ip-field");
        row(3, 2, "Port: " + _port, "port-field");
        row(4, 3, "Connect", "connect");
    } else if (_state.page == FrontendPage::Connecting || _state.page == FrontendPage::Waiting) {
        row(5, 2, "Cancel", "cancel");
    } else if (_state.page == FrontendPage::Error) {
        row(4, 2, "Retry", "retry");
        row(6, 3, "Edit address", "back");
    } else if (_state.page == FrontendPage::Lobby) {
        const float modeWidth = 280.0F * s;
        const float modeGap = 32.0F * s;
        const float start = (viewport.x - 3.0F * modeWidth - 2.0F * modeGap) / 2.0F;
        const bool allowed = _state.leader && !_state.selectionPending;
        for (int index = 0; index < 3; ++index) {
            constexpr const char* names[]{"Campaign", "Duel", "Infinite"};
            constexpr const char* art[]{"campaign", "duel", "infinite"};
            const bool enabled = allowed && index != 1 && (index != 0 || _state.unlocked > 0);
            result.push_back({10 + index,
                              {{start + static_cast<float>(index) * (modeWidth + modeGap), top},
                               modeWidth,
                               150.0F * s},
                              names[index],
                              art[index],
                              enabled});
        }
        const float second = top + 174.0F * s;
        if (_state.mode == FrontendMode::Campaign && !_state.levels.empty()) {
            result.push_back({13,
                              {{left, second}, 44.0F * s, 44.0F * s},
                              "<",
                              "previous",
                              allowed && _state.level > 0});
            result.push_back({14,
                              {{left + width - 44.0F * s, second}, 44.0F * s, 44.0F * s},
                              ">",
                              "next",
                              allowed && _state.level + 1 < _state.unlocked &&
                                  _state.level + 1 < _state.levels.size()});
        }
        result.push_back({15,
                          {{left, second + 64.0F * s}, width, height},
                          _state.selectionPending ? "Waiting..." : "Start",
                          "start",
                          allowed && _state.mode != FrontendMode::Duel &&
                              (_state.mode != FrontendMode::Campaign || _state.unlocked > 0)});
        result.push_back({16,
                          {{viewport.x - 180.0F * s - 16.0F, viewport.y - 44.0F * s - 12.0F},
                           180.0F * s,
                           44.0F * s},
                          "Disconnect",
                          "disconnect"});
    }
    return result;
}

FrontendAction FrontendMenu::activate(int id)
{
    if (id == 0) {
        _settingsRequested = true;
        return {};
    }
    if (id == 9)
        return command(FrontendCommand::Quit);
    if (id == 4) {
        unsigned port = 0;
        const auto parsed = std::from_chars(_port.data(), _port.data() + _port.size(), port);
        if (_entry.name.empty() || _entry.name.size() > 32 || !validAddress(_entry.host) ||
            parsed.ec != std::errc{} || parsed.ptr != _port.data() + _port.size() || port == 0 ||
            port > 65535) {
            _notice = "Enter a name, an IPv4 address and a port from 1 to 65535.";
            return {};
        }
        _entry.port = static_cast<std::uint16_t>(port);
        auto action = command(FrontendCommand::Connect);
        action.connection = _entry;
        return action;
    }
    if (id == 5 || id == 6)
        return command(FrontendCommand::Cancel);
    if (id == 16)
        return command(FrontendCommand::Disconnect);
    auto action = command(id == 15 ? FrontendCommand::Start : FrontendCommand::Select);
    action.mode = _state.mode;
    action.level = _state.level;
    if (id == 10)
        action.mode = FrontendMode::Campaign;
    if (id == 12)
        action.mode = FrontendMode::Infinite;
    if (id == 13 && action.level > 0)
        --action.level;
    if (id == 14)
        ++action.level;
    return action;
}

FrontendAction FrontendMenu::handle(const InputFrame& frame, GuiVector2 viewport, float scale)
{
    if (!frame.keyboard.focused)
        return {};
    const auto rows = controls(viewport, scale);
    _hovered = -1;
    for (const auto& row : rows)
        if (row.enabled && contains(row.bounds, frame.mouse))
            _hovered = row.id;
    const auto selected = std::find_if(rows.begin(), rows.end(), [&](const auto& row) {
        return row.id == _selected && row.enabled;
    });
    if (selected == rows.end()) {
        const auto first =
            std::find_if(rows.begin(), rows.end(), [](const auto& row) { return row.enabled; });
        if (first != rows.end())
            _selected = first->id;
    }
    if (frame.mouseClick) {
        for (const auto& row : rows) {
            if (row.enabled && contains(row.bounds, frame.mouse)) {
                _selected = row.id;
                return row.id >= 1 && row.id <= 3 ? FrontendAction{} : activate(row.id);
            }
        }
    }
    const bool editing =
        _state.page == FrontendPage::Connection && _selected >= 1 && _selected <= 3;
    if (editing) {
        auto& target = _selected == 1 ? _entry.name : _selected == 2 ? _entry.host : _port;
        if (frame.backspace && !target.empty())
            target.pop_back();
        const std::size_t limit = _selected == 1 ? 32 : _selected == 2 ? 15 : 5;
        for (const char character : frame.text) {
            const bool digit = character >= '0' && character <= '9';
            if (target.size() < limit &&
                (_selected == 1 || digit || (_selected == 2 && character == '.')))
                target.push_back(character);
        }
        if (frame.confirm)
            return activate(4);
    }
    const int direction = frame.tab ? (frame.shift ? -1 : 1) : frame.menu.vertical;
    if (direction != 0) {
        std::vector<int> enabled;
        for (const auto& row : rows)
            if (row.enabled)
                enabled.push_back(row.id);
        std::sort(enabled.begin(), enabled.end(), [](int a, int b) {
            const auto order = [](int id) { return id == 0 ? 100 : id == 9 ? 101 : id; };
            return order(a) < order(b);
        });
        const auto it = std::find(enabled.begin(), enabled.end(), _selected);
        const int index = it == enabled.end() ? 0 : static_cast<int>(it - enabled.begin());
        if (!enabled.empty())
            _selected = enabled[static_cast<std::size_t>(
                (index + direction + static_cast<int>(enabled.size())) %
                static_cast<int>(enabled.size()))];
    } else if (!editing && (frame.confirm || frame.menu.activate)) {
        return activate(_selected);
    }
    if (frame.escape &&
        (_state.page == FrontendPage::Connecting || _state.page == FrontendPage::Waiting ||
         _state.page == FrontendPage::Error))
        return command(FrontendCommand::Cancel);
    return {};
}

void FrontendMenu::text(RenderScene& scene, const std::string& value, GuiRectangle bounds,
                        float size, GuiColor color) const
{
    size = std::max(12.0F, size);
    std::string displayed = value;
    const auto measure = [&](const std::string& content) {
        return _measure ? _measure(content, size)
                        : static_cast<float>(content.size()) * size * 0.6F;
    };
    const float available = std::max(0.0F, bounds.width - 12.0F);
    while (size > 12.0F && measure(displayed) > available)
        size -= 1.0F;
    if (measure(displayed) > available) {
        while (!displayed.empty() && measure(displayed + "...") > available)
            displayed.pop_back();
        if (!displayed.empty())
            displayed += "...";
    }
    TextDraw draw;
    draw.fontId = _font;
    draw.text = displayed;
    draw.position = {bounds.pos.x + (bounds.width - measure(displayed)) / 2.0F,
                     bounds.pos.y + (bounds.height - size) / 2.0F};
    draw.fontSize = size;
    draw.spacing = 1.0F;
    draw.color = color;
    scene.overlayTexts.push_back(std::move(draw));
}

RenderScene FrontendMenu::makeScene(GuiVector2 viewport, float scale) const
{
    RenderScene scene;
    scene.clearColor = {34, 35, 40, 255};
    const float s = layoutScale(viewport);
    const float size = std::max(16.0F, 28.0F * s) * scale;
    const auto artwork = [&](const std::string& id, GuiRectangle bounds, GuiColor tint) {
        const auto found = _artwork.find(id);
        if (found == _artwork.end())
            return false;
        const auto source = found->second;
        const float fit = std::min(bounds.width / source.x, bounds.height / source.y);
        SpriteDraw sprite;
        sprite.textureId = "ui-" + id;
        sprite.sourceRect = {{0, 0}, source.x, source.y};
        sprite.destinationRect = {{bounds.pos.x + (bounds.width - source.x * fit) / 2.0F,
                                   bounds.pos.y + (bounds.height - source.y * fit) / 2.0F},
                                  source.x * fit,
                                  source.y * fit};
        sprite.tint = tint;
        scene.sprites.push_back(std::move(sprite));
        return true;
    };
    if (_artwork.contains("background")) {
        SpriteDraw background;
        background.textureId = "ui-background";
        const auto source = _artwork.at("background");
        background.sourceRect = {{0, 0}, source.x, source.y};
        background.destinationRect = {{0, 0}, viewport.x, viewport.y};
        background.layer = -1000;
        scene.sprites.push_back(std::move(background));
    }
    const GuiRectangle title{{(viewport.x - 360.0F * s) / 2.0F, 26.0F * s}, 360.0F * s, 100.0F * s};
    if (!artwork("title", title, {})) {
        scene.rectangles.push_back({title, {255, 255, 255, 255}});
        text(scene, "R-Type", title, size * 1.4F, {20, 20, 20, 255});
    }
    for (const auto& row : controls(viewport, scale)) {
        if (row.id == _selected && row.enabled) {
            const auto& b = row.bounds;
            const GuiColor focus{255, 255, 255, 255};
            scene.rectangles.push_back({{{b.pos.x - 4, b.pos.y - 4}, b.width + 8, 2}, focus});
            scene.rectangles.push_back(
                {{{b.pos.x - 4, b.pos.y + b.height + 2}, b.width + 8, 2}, focus});
        }
        const GuiColor tint = !row.enabled         ? GuiColor{155, 155, 155, 255}
                              : row.id == _hovered ? GuiColor{225, 225, 225, 255}
                                                   : GuiColor{};
        std::string art = row.artwork;
        if (!_artwork.contains(art)) {
            const bool field = row.id >= 1 && row.id <= 3;
            if (field)
                art = "input";
            else if (row.id != 0 && (row.id < 10 || row.id > 12))
                art = "button";
        }
        const bool image = artwork(art, row.bounds, tint);
        if (!image)
            scene.rectangles.push_back({row.bounds, tint});
        if (!image || row.artwork != "settings")
            text(scene, row.id == 0 ? "*" : row.label, row.bounds, size, {20, 20, 20, 255});
    }
    if (_state.page == FrontendPage::Lobby) {
        const float top = viewport.y * 0.30F;
        const std::string selected = _state.mode == FrontendMode::Campaign   ? "Campaign selected"
                                     : _state.mode == FrontendMode::Infinite ? "Infinite selected"
                                                                             : "Duel unavailable";
        text(scene, selected + (_state.leader ? "" : " / Waiting for leader"),
             {{16, top - 48.0F * s}, viewport.x - 32, 40.0F * s}, size * 0.7F, {});
        text(scene, "Duel unavailable",
             {{viewport.x / 2.0F - 140.0F * s, top + 152.0F * s}, 280.0F * s, 24.0F * s},
             size * 0.6F, {});
        if (_state.mode == FrontendMode::Campaign && _state.level < _state.levels.size())
            text(scene, _state.levels[_state.level],
                 {{viewport.x / 2.0F - 200.0F * s, top + 174.0F * s}, 400.0F * s, 44.0F * s},
                 size * 0.8F, {});
    }
    if (_state.page == FrontendPage::Connecting || _state.page == FrontendPage::Waiting ||
        _state.page == FrontendPage::Error) {
        const char* heading = _state.page == FrontendPage::Error     ? "Connection failed"
                              : _state.page == FrontendPage::Waiting ? "Waiting for the server"
                                                                     : "Connecting...";
        text(scene, heading, {{16, viewport.y * 0.31F}, viewport.x - 32, 50.0F * s}, size, {});
        text(scene, _state.message, {{16, viewport.y * 0.41F}, viewport.x - 32, 50.0F * s},
             size * 0.8F, {});
    }
    const std::string notice = _notice.empty() ? (_state.page == FrontendPage::Lobby ||
                                                          _state.page == FrontendPage::Connection
                                                      ? _state.message
                                                      : "")
                                               : _notice;
    text(scene, notice,
         {{130.0F * s + 16.0F, viewport.y - 48.0F * s - 12.0F},
          std::max(1.0F, viewport.x - 330.0F * s - 32.0F),
          44.0F * s},
         size * 0.65F, {});
    return scene;
}

} // namespace rtype::gui
