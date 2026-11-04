#include "FrontendMenu.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace rtype::gui;
namespace {
void check(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error(message);
}
FrontendAction click(FrontendMenu& menu, int id, GuiVector2 viewport = {1280, 720})
{
    const auto controls = menu.controls(viewport, 1);
    const auto it = std::find_if(controls.begin(), controls.end(),
                                 [id](const auto& item) { return item.id == id; });
    check(it != controls.end(), "Control exists");
    InputFrame input;
    input.mouseClick = true;
    input.mouse = {it->bounds.pos.x + it->bounds.width / 2,
                   it->bounds.pos.y + it->bounds.height / 2};
    return menu.handle(input, viewport, 1);
}
} // namespace
int main()
{
    try {
        FrontendMenu menu;
        FrontendState state;
        state.page = FrontendPage::Connection;
        menu.setState(state);
        check(menu.active(), "Connection suppresses gameplay");
        auto action = click(menu, 4);
        check(action.command == FrontendCommand::Connect && action.connection.host == "127.0.0.1" &&
                  action.connection.port == 4242,
              "Connection details submitted");
        (void)click(menu, 3);
        InputFrame input;
        input.backspace = true;
        for (int i = 0; i < 4; ++i)
            (void)menu.handle(input, {1280, 720}, 1);
        check(click(menu, 4).command == FrontendCommand::None, "Empty port refused");
        (void)click(menu, 3);
        input = {};
        input.text = "65536";
        (void)menu.handle(input, {1280, 720}, 1);
        check(click(menu, 4).command == FrontendCommand::None, "Out of range port refused");
        menu.reset();
        auto invalid = state;
        invalid.connection.host = "999.0.0.1";
        menu.setState(invalid);
        check(click(menu, 4).command == FrontendCommand::None, "Invalid IPv4 address refused");
        menu.reset();
        menu.setState(state);
        (void)click(menu, 1);
        input = {};
        input.text = " X";
        input.menu.activate = true;
        check(menu.handle(input, {1280, 720}, 1).command == FrontendCommand::None,
              "Space in name does not submit form");
        input = {};
        input.confirm = true;
        action = menu.handle(input, {1280, 720}, 1);
        check(action.connection.name == "Player X", "Name typed before submission");
        input = {};
        input.escape = true;
        state.page = FrontendPage::Connecting;
        menu.setState(state);
        check(menu.handle(input, {1280, 720}, 1).command == FrontendCommand::Cancel,
              "Escape cancels connecting");
        state.page = FrontendPage::Lobby;
        state.leader = true;
        state.unlocked = 2;
        state.levels = {"Stage 1", "Stage 2"};
        menu.setState(state);
        const auto modeControls = menu.controls({1280, 720}, 1);
        check(std::none_of(modeControls.begin(), modeControls.end(),
                           [](const auto& row) { return row.id == 11; }),
              "Only Campaign and Infinite are offered");
        check(click(menu, 12).mode == FrontendMode::Infinite, "Mode selection forwarded");
        check(click(menu, 14).level == 1, "Next unlocked level requested");
        state.unlocked = 1;
        menu.setState(state);
        check(click(menu, 14).command == FrontendCommand::None, "Locked next level disabled");
        state.selectionPending = true;
        menu.setState(state);
        check(click(menu, 15).command == FrontendCommand::None,
              "Start disabled before server confirmation");
        state.selectionPending = false;
        state.leader = false;
        menu.setState(state);
        check(click(menu, 10).command == FrontendCommand::None &&
                  click(menu, 15).command == FrontendCommand::None,
              "Followers cannot select or start");
        (void)click(menu, 0);
        check(menu.takeSettingsRequest() && !menu.takeSettingsRequest(),
              "Settings request consumed once");
        for (const auto viewport :
             {GuiVector2{640, 360}, GuiVector2{1280, 720}, GuiVector2{1920, 1080}}) {
            for (const float scale : {1.0F, 1.25F, 1.5F}) {
                for (const auto page : {FrontendPage::Connection, FrontendPage::Lobby,
                                        FrontendPage::Connecting, FrontendPage::Error}) {
                    state.page = page;
                    menu.setState(state);
                    const auto scene = menu.makeScene(viewport, scale);
                    for (const auto& rectangle : scene.rectangles) {
                        const auto& b = rectangle.bounds;
                        check(b.pos.x >= 0 && b.pos.y >= 0 &&
                                  b.pos.x + b.width <= viewport.x + 0.01F &&
                                  b.pos.y + b.height <= viewport.y + 0.01F,
                              "Controls fit supported viewport");
                    }
                    for (const auto& text : scene.overlayTexts)
                        check(std::isfinite(text.position.x) && std::isfinite(text.position.y) &&
                                  text.position.y >= 0,
                              "Text geometry valid");
                }
            }
        }
        menu.setArtwork("title", {200, 60});
        check(!menu.makeScene({1280, 720}, 1).sprites.empty(), "Sprite replaces placeholder");
        FrontendMenu focusMenu;
        state.page = FrontendPage::Lobby;
        state.leader = true;
        focusMenu.setState(state);
        focusMenu.setArtwork("paper-mode", {1214, 690});
        focusMenu.setArtwork("paper-button", {3500, 425});
        for (int step = 0; step < 6; ++step) {
            InputFrame tab;
            tab.tab = true;
            (void)focusMenu.handle(tab, {1280, 720}, 1);
            int focused = 0;
            const auto focusedScene = focusMenu.makeScene({1280, 720}, 1);
            const auto warm = [](GuiColor color) {
                return color.red == 245 && color.green == 226 && color.blue == 190;
            };
            for (const auto& sprite : focusedScene.sprites)
                focused += warm(sprite.tint) ? 1 : 0;
            for (const auto& rectangle : focusedScene.rectangles)
                focused += warm(rectangle.color) ? 1 : 0;
            check(focused == 1,
                  "Tab highlights only one control, independently of the active mode");
        }
        state.page = FrontendPage::Gameplay;
        menu.setState(state);
        check(!menu.active(), "Gameplay leaves frontend");
        std::cout << "Frontend menu tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
