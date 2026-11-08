#include "FrontendMenu.hpp"

#include "MenuArtwork.hpp"

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
std::string spaced(std::uint32_t value)
{
    const std::string digits = std::to_string(value);
    std::string out;
    for (std::size_t i = 0; i < digits.size(); ++i) {
        if (i > 0 && (digits.size() - i) % 3 == 0)
            out += ' ';
        out += digits[i];
    }
    return out;
}
constexpr int BackToMenuId = 20;
constexpr int NextStageId = 21;
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
void FrontendMenu::setResults(const ResultsScreen& results)
{
    if (!results.visible)
        _resultsDismissed = false;
    else if (!_results.visible) {
        _resultsDismissed = false;
        _selected = results.nextStage ? NextStageId : BackToMenuId;
    }
    _results = results;
}
FrontendMenu::ResultsLayout FrontendMenu::resultsLayout(GuiVector2 viewport) const
{
    const float s = layoutScale(viewport);
    const float rows = static_cast<float>(std::max<std::size_t>(1, _results.rows.size()));
    const float height = (352.0F + 46.0F * rows) * s;
    const float width = std::min(viewport.x - 32.0F, std::max(640.0F * s, height * 1.39F));
    ResultsLayout layout;
    layout.panel = {
        {(viewport.x - width) / 2.0F, std::max(8.0F, (viewport.y - height) / 2.0F)}, width, height};
    const float buttonY = layout.panel.pos.y + (252.0F + 46.0F * rows) * s;
    const float buttonHeight = 60.0F * s;
    if (_results.nextStage) {
        const float nextWidth = 250.0F * s;
        const float backWidth = 230.0F * s;
        const float gap = 20.0F * s;
        const float start = viewport.x / 2.0F - (nextWidth + gap + backWidth) / 2.0F;
        layout.next = {{start, buttonY}, nextWidth, buttonHeight};
        layout.button = {{start + nextWidth + gap, buttonY}, backWidth, buttonHeight};
    } else {
        layout.button = {{viewport.x / 2.0F - 140.0F * s, buttonY}, 280.0F * s, buttonHeight};
    }
    return layout;
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
    const float height = std::max(34.0F, 68.0F * s);
    const float gap = std::max(8.0F, 16.0F * s);
    const float top = viewport.y * 0.30F;
    std::vector<FrontendControl> result{
        {0, detail::settingsButtonBounds(viewport), "Settings", "settings"},
        {9, {{16.0F, viewport.y - 44.0F * s - 12.0F}, 100.0F * s, 44.0F * s}, "Quit", "quit"}};
    if (resultsActive()) {
        const auto layout = resultsLayout(viewport);
        if (_results.nextStage)
            result.push_back({NextStageId, layout.next, "Next stage", "button",
                              _state.leader && !_state.selectionPending});
        result.push_back({BackToMenuId, layout.button, "Back to menu", "button"});
        return result;
    }
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
        const float start = (viewport.x - 2.0F * modeWidth - modeGap) / 2.0F;
        const bool allowed = _state.leader && !_state.selectionPending;
        for (int index = 0; index < 2; ++index) {
            constexpr const char* names[]{"Campaign", "Infinite"};
            constexpr const char* art[]{"campaign", "infinite"};
            const bool enabled = allowed && (index != 0 || _state.unlocked > 0);
            result.push_back({index == 0 ? 10 : 12,
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
    if (id == BackToMenuId) {
        _resultsDismissed = true;
        _selected = 10;
        return {};
    }
    if (id == NextStageId) {
        _resultsDismissed = true;
        auto action = command(FrontendCommand::Start);
        action.mode = _state.mode;
        action.level = _state.level;
        return action;
    }
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
        if (frame.confirm || (frame.menu.activate && frame.menu.gamepadButton))
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
    if ((frame.escape || frame.menu.back) &&
        (_state.page == FrontendPage::Connecting || _state.page == FrontendPage::Waiting ||
         _state.page == FrontendPage::Error))
        return command(FrontendCommand::Cancel);
    return {};
}

void FrontendMenu::text(RenderScene& scene, const std::string& value, GuiRectangle bounds,
                        float size, GuiColor color) const
{
    size = std::clamp(size, 12.0F, std::max(12.0F, bounds.height * 0.60F));
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
                     bounds.pos.y + (bounds.height - size) / 2.0F + size * 0.24F};
    draw.fontSize = size;
    draw.spacing = 1.0F;
    draw.color = color;
    scene.overlayTexts.push_back(std::move(draw));
}

void FrontendMenu::drawResults(RenderScene& scene, GuiVector2 viewport) const
{
    constexpr GuiColor ink{30, 30, 35, 255};
    constexpr GuiColor muted{115, 108, 95, 255};
    constexpr GuiColor gold{205, 130, 15, 255};
    const float s = layoutScale(viewport);
    const auto& panel = resultsLayout(viewport).panel;
    const float pad = 60.0F * s;
    const float left = panel.pos.x + pad;
    const float right = panel.pos.x + panel.width - pad;
    const float killsRight = right - 190.0F * s;
    const float centerX = panel.pos.x + panel.width / 2.0F;
    const float top = panel.pos.y;

    const auto measure = [this](const std::string& v, float size) {
        return _measure ? _measure(v, size) : static_cast<float>(v.size()) * size * 0.6F;
    };
    const auto put = [&](const std::string& v, float x, float y, float size, GuiColor c,
                         float align = 0.0F) {
        TextDraw d;
        d.fontId = _font;
        d.text = v;
        d.position = {x - measure(v, size) * align, y + size * 0.2F};
        d.fontSize = size;
        d.color = c;
        scene.overlayTexts.push_back(std::move(d));
    };

    if (const auto sheet = _artwork.find("paper-score"); sheet != _artwork.end()) {
        const float w = panel.width / 0.839F;
        const float h = panel.height / 0.809F;
        SpriteDraw paper;
        paper.textureId = "ui-paper-score";
        paper.sourceRect = {{0.0F, 0.0F}, sheet->second.x, sheet->second.y};
        paper.destinationRect = {{panel.pos.x - 0.086F * w, top - 0.099F * h}, w, h};
        scene.sprites.push_back(std::move(paper));
    } else {
        scene.rectangles.push_back({panel, {255, 252, 245, 255}});
    }

    put(_results.subtitle, centerX, top + 40.0F * s, std::max(12.0F, 16.0F * s), muted, 0.5F);
    float titleSize = std::max(16.0F, 54.0F * s);
    while (titleSize > 16.0F && measure(_results.title, titleSize) > panel.width - 2.0F * pad)
        titleSize -= 1.0F;
    put(_results.title, centerX, top + 66.0F * s, titleSize, ink, 0.5F);

    const float head = std::max(11.0F, 13.0F * s);
    put("PLAYER", left, top + 152.0F * s, head, muted);
    put("KILLS", killsRight, top + 152.0F * s, head, muted, 1.0F);
    put("SCORE", right, top + 152.0F * s, head, muted, 1.0F);

    const float rowHeight = 46.0F * s;
    const float size = std::max(14.0F, 24.0F * s);
    const float tagSize = std::max(11.0F, 14.0F * s);
    float y = top + 178.0F * s;
    for (const auto& row : _results.rows) {
        const float textY = y + (rowHeight - size) / 2.0F;
        const float tagY = textY + (size - tagSize) / 2.0F;
        float tagX = left + measure(row.name, size) + 10.0F * s;
        put(row.name, left, textY, size, ink);
        if (row.mvp) {
            put("MVP", tagX, tagY, tagSize, gold);
            tagX += measure("MVP", tagSize) + 10.0F * s;
        }
        if (row.you)
            put("YOU", tagX, tagY, tagSize, muted);
        put(std::to_string(row.kills), killsRight, textY, size, ink, 1.0F);
        put(spaced(row.score), right, textY, size, ink, 1.0F);
        y += rowHeight;
    }

    const float totalSize = std::max(20.0F, 40.0F * s);
    const float labelSize = std::max(12.0F, 15.0F * s);
    const float totalY = y + 14.0F * s;
    const std::string total = spaced(_results.total);
    put(total, right, totalY, totalSize, ink, 1.0F);
    put("TOTAL", right - measure(total, totalSize) - 16.0F * s,
        totalY + (totalSize - labelSize) / 2.0F + 2.0F * s, labelSize, muted, 1.0F);
}

RenderScene FrontendMenu::makeScene(GuiVector2 viewport, float scale) const
{
    RenderScene scene;
    scene.clearColor = {248, 246, 235, 255};
    constexpr GuiColor ink{30, 30, 35, 255};
    const float s = layoutScale(viewport);
    const float size = std::max(16.0F, 24.0F * s) * scale;
    const auto artwork = [&](const std::string& id, GuiRectangle bounds, GuiColor tint) {
        return detail::menuArtwork(scene, _artwork, id, bounds, tint);
    };
    (void)artwork("background", {{0, 0}, viewport.x, viewport.y}, {});
    const GuiRectangle title{{(viewport.x - 360.0F * s) / 2.0F, 16.0F * s}, 360.0F * s, 100.0F * s};
    if (!artwork("title", title, {})) {
        text(scene, "R-Type", title, size * 1.4F, {20, 20, 20, 255});
    }
    if (resultsActive())
        drawResults(scene, viewport);
    for (const auto& row : controls(viewport, scale)) {
        const GuiColor tint = !row.enabled          ? GuiColor{190, 190, 190, 255}
                              : row.id == _selected ? GuiColor{245, 226, 190, 255}
                              : row.id == _hovered  ? GuiColor{230, 230, 220, 255}
                                                    : GuiColor{};
        std::string art = row.artwork;
        if (_artwork.find("background") != _artwork.end()) {
            const bool field = row.id >= 1 && row.id <= 3;
            if (field)
                art = "input";
            else if (row.id != 0 && (row.id < 10 || row.id > 12))
                art = "button";
        }
        const bool coolerPaper =
            row.artwork == "quit" || row.artwork == "campaign" || row.artwork == "infinite";
        const auto paperTint =
            coolerPaper && row.enabled && row.id != _selected ? detail::warmPaper(tint) : tint;
        bool image = artwork(art, row.bounds, paperTint);
        if (!image && row.id != 0)
            image = artwork(detail::paperArtwork(_artwork, row.bounds), row.bounds, paperTint);
        if (!image)
            scene.rectangles.push_back({row.bounds, tint});
        if (!image || row.artwork != "settings") {
            GuiRectangle label = image ? scene.sprites.back().destinationRect : row.bounds;
            label.pos.x += label.width * 0.10F;
            label.width *= 0.80F;
            text(scene, row.id == 0 ? "*" : row.label, label, size, ink);
        }
    }
    if (_state.page == FrontendPage::Lobby && !resultsActive()) {
        const float top = viewport.y * 0.30F;
        const std::string selected = _state.mode == FrontendMode::Campaign   ? "Campaign selected"
                                     : _state.mode == FrontendMode::Infinite ? "Infinite selected"
                                                                             : "Choose a game mode";
        text(scene, selected + (_state.leader ? "" : " / Waiting for leader"),
             {{16, top - 48.0F * s}, viewport.x - 32, 40.0F * s}, size * 0.7F, ink);
        if (_state.mode == FrontendMode::Campaign && _state.level < _state.levels.size())
            text(scene, _state.levels[_state.level],
                 {{viewport.x / 2.0F - 200.0F * s, top + 174.0F * s}, 400.0F * s, 44.0F * s},
                 size * 0.8F, ink);
    }
    if (_state.page == FrontendPage::Connecting || _state.page == FrontendPage::Waiting ||
        _state.page == FrontendPage::Error) {
        const char* heading = _state.page == FrontendPage::Error     ? "Connection failed"
                              : _state.page == FrontendPage::Waiting ? "Waiting for the server"
                                                                     : "Connecting...";
        text(scene, heading, {{16, viewport.y * 0.31F}, viewport.x - 32, 50.0F * s}, size, ink);
        text(scene, _state.message, {{16, viewport.y * 0.41F}, viewport.x - 32, 50.0F * s},
             size * 0.8F, ink);
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
         size * 0.65F, ink);
    return scene;
}

} // namespace rtype::gui
